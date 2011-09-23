/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "raul/log.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "events/Disconnect.hpp"
#include "AudioBuffer.hpp"
#include "ClientBroadcaster.hpp"
#include "ConnectionImpl.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "Request.hpp"
#include "ThreadManager.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {
namespace Events {

Disconnect::Disconnect(
		Engine&            engine,
		SharedPtr<Request> request,
		SampleCount        timestamp,
		const Raul::Path&  src_port_path,
		const Raul::Path&  dst_port_path)
	: QueuedEvent(engine, request, timestamp)
	, _src_port_path(src_port_path)
	, _dst_port_path(dst_port_path)
	, _patch(NULL)
	, _src_port(NULL)
	, _dst_port(NULL)
	, _impl(NULL)
	, _compiled_patch(NULL)
{
}

Disconnect::Impl::Impl(Engine&     e,
                       PatchImpl*  patch,
                       OutputPort* s,
                       InputPort*  d)
	: _engine(e)
	, _src_output_port(s)
	, _dst_input_port(d)
	, _patch(patch)
	, _connection(patch->remove_connection(_src_output_port, _dst_input_port))
	, _buffers(NULL)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	NodeImpl* const src_node = _src_output_port->parent_node();
	NodeImpl* const dst_node = _dst_input_port->parent_node();

	for (Raul::List<NodeImpl*>::iterator i = dst_node->providers()->begin();
	     i != dst_node->providers()->end(); ++i) {
		if ((*i) == src_node) {
			delete dst_node->providers()->erase(i);
			break;
		}
	}

	for (Raul::List<NodeImpl*>::iterator i = src_node->dependants()->begin();
	     i != src_node->dependants()->end(); ++i) {
		if ((*i) == dst_node) {
			delete src_node->dependants()->erase(i);
			break;
		}
	}

	_dst_input_port->decrement_num_connections();

	if (_dst_input_port->num_connections() == 0) {
		_buffers = new Raul::Array<BufferFactory::Ref>(_dst_input_port->poly());
		_dst_input_port->get_buffers(*_engine.buffer_factory(),
				_buffers, _dst_input_port->poly());

		const bool  is_control = _dst_input_port->is_a(PortType::CONTROL);
		const float value      = is_control ? _dst_input_port->value().get_float() : 0;
		for (uint32_t i = 0; i < _buffers->size(); ++i) {
			if (is_control) {
				PtrCast<AudioBuffer>(_buffers->at(i))->set_value(value, 0, 0);
			} else {
				_buffers->at(i)->clear();
			}
		}
	}

	_connection->pending_disconnection(true);
}

void
Disconnect::pre_process()
{
	if (_src_port_path.parent().parent() != _dst_port_path.parent().parent()
	    && _src_port_path.parent() != _dst_port_path.parent().parent()
	    && _src_port_path.parent().parent() != _dst_port_path.parent()) {
		_error = PARENT_PATCH_DIFFERENT;
		QueuedEvent::pre_process();
		return;
	}

	_src_port = _engine.engine_store()->find_port(_src_port_path);
	_dst_port = _engine.engine_store()->find_port(_dst_port_path);

	if (_src_port == NULL || _dst_port == NULL) {
		_error = PORT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	NodeImpl* const src_node = _src_port->parent_node();
	NodeImpl* const dst_node = _dst_port->parent_node();

	// Connection to a patch port from inside the patch
	if (src_node->parent_patch() != dst_node->parent_patch()) {

		assert(src_node->parent() == dst_node || dst_node->parent() == src_node);
		if (src_node->parent() == dst_node)
			_patch = dynamic_cast<PatchImpl*>(dst_node);
		else
			_patch = dynamic_cast<PatchImpl*>(src_node);

	// Connection from a patch input to a patch output (pass through)
	} else if (src_node == dst_node && dynamic_cast<PatchImpl*>(src_node)) {
		_patch = dynamic_cast<PatchImpl*>(src_node);

	// Normal connection between nodes with the same parent
	} else {
		_patch = src_node->parent_patch();
	}

	assert(_patch);

	if (!_patch->has_connection(_src_port, _dst_port)) {
		_error = NOT_CONNECTED;
		QueuedEvent::pre_process();
		return;
	}

	if (src_node == NULL || dst_node == NULL) {
		_error = PARENTS_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	_impl = new Impl(_engine,
	                 _patch,
	                 dynamic_cast<OutputPort*>(_src_port),
	                 dynamic_cast<InputPort*>(_dst_port));

	if (_patch->enabled())
		_compiled_patch = _patch->compile();

	QueuedEvent::pre_process();
}

bool
Disconnect::Impl::execute(ProcessContext& context, bool set_dst_buffers)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	InputPort::Connections::Node* const port_connections_node
		= _dst_input_port->remove_connection(context, _src_output_port);
	if (!port_connections_node) {
		return false;
	}

	if (set_dst_buffers) {
		if (_buffers) {
			_engine.maid()->push(_dst_input_port->set_buffers(_buffers));
		} else {
			_dst_input_port->setup_buffers(*_engine.buffer_factory(),
			                               _dst_input_port->poly());
		}
		_dst_input_port->connect_buffers();
	} else {
		_dst_input_port->recycle_buffers();
	}

	assert(_connection);
	assert(port_connections_node->elem() == _connection);

	_engine.maid()->push(port_connections_node);
	return true;
}

void
Disconnect::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_error == NO_ERROR) {
		if (!_impl->execute(context, true)) {
			_error = CONNECTION_NOT_FOUND;
			return;
		}

		_engine.maid()->push(_patch->compiled_patch());
		_patch->compiled_patch(_compiled_patch);
	}
}

void
Disconnect::post_process()
{
	if (_error == NO_ERROR) {
		if (_request)
			_request->respond_ok();
		_engine.broadcaster()->disconnect(_src_port->path(), _dst_port->path());
	} else {
		string msg("Unable to disconnect ");
		msg.append(_src_port_path.str() + " => " + _dst_port_path.str());
		msg.append(" (");
		switch (_error) {
		case PARENT_PATCH_DIFFERENT:
			msg.append("Ports exist in different patches");
			break;
		case PORT_NOT_FOUND:
			msg.append("Port not found");
			break;
		case TYPE_MISMATCH:
			msg.append("Ports have incompatible types");
			break;
		case NOT_CONNECTED:
			msg.append("Ports are not connected");
			break;
		case PARENTS_NOT_FOUND:
			msg.append("Parent node not found");
			break;
		case CONNECTION_NOT_FOUND:
			msg.append("Connection not found");
			break;
		default:
			break;
		}
		msg.append(")");
		if (_request)
			_request->respond_error(msg);
	}

	delete _impl;
}

} // namespace Server
} // namespace Ingen
} // namespace Events
