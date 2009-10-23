/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include "raul/Maid.hpp"
#include "ClearPatch.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "PatchImpl.hpp"
#include "ClientBroadcaster.hpp"
#include "util.hpp"
#include "EngineStore.hpp"
#include "PortImpl.hpp"
#include "NodeImpl.hpp"
#include "ConnectionImpl.hpp"
#include "QueuedEventSource.hpp"
#include "AudioDriver.hpp"
#include "MidiDriver.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Events {

using namespace Shared;


ClearPatch::ClearPatch(Engine& engine, SharedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, const Path& patch_path)
	: QueuedEvent(engine, responder, time, true, source)
	, _patch_path(patch_path)
	, _process(false)
	, _ports_array(NULL)
	, _compiled_patch(NULL)
	, _driver_ports(NULL)
{
}


void
ClearPatch::pre_process()
{
	EngineStore::Objects::iterator patch_iterator = _engine.engine_store()->find(_patch_path);

	if (patch_iterator != _engine.engine_store()->end()) {
		_patch = PtrCast<PatchImpl>(patch_iterator->second);
		if (_patch) {
			_process = _patch->enabled();
			_removed_table = _engine.engine_store()->remove_children(patch_iterator);
			_patch->nodes().clear();
			_patch->connections().clear();
			_patch->clear_ports();
			_ports_array = _patch->build_ports_array();
			if (_patch->enabled())
				_compiled_patch = _patch->compile();

			// Remove driver ports
			if (_patch->parent() == NULL) {
				size_t port_count = 0;
				for (EngineStore::Objects::iterator i = _removed_table->begin();
						i != _removed_table->end(); ++i) {
					SharedPtr<PortImpl> port = PtrCast<PortImpl>(i->second);
					if (port)
						++port_count;

					SharedPtr<NodeImpl> node = PtrCast<NodeImpl>(i->second);
					if (node)
						node->deactivate();
				}

				_driver_ports = new DriverPorts(port_count, NULL);
			}
		}
	}

	QueuedEvent::pre_process();
}


void
ClearPatch::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_patch && _removed_table) {
		_patch->disable();

		if (_patch->compiled_patch() != NULL) {
			_engine.maid()->push(_patch->compiled_patch());
			_patch->compiled_patch(NULL);
		}

		_patch->connections().clear();
		_patch->compiled_patch(_compiled_patch);
		Raul::Array<PortImpl*>* old_ports = _patch->external_ports();
		_patch->external_ports(_ports_array);
		_ports_array = old_ports;

		// Remove driver ports
		if (_patch->parent() == NULL) {
			for (EngineStore::Objects::iterator i = _removed_table->begin();
					i != _removed_table->end(); ++i) {
				SharedPtr<PortImpl> port = PtrCast<PortImpl>(i->second);
				if (port && port->type() == DataType::AUDIO) {
					_driver_ports->push_back(
							_engine.audio_driver()->remove_port(port->path()));
				} else if (port && port->type() == DataType::EVENT) {
					_driver_ports->push_back(
							_engine.midi_driver()->remove_port(port->path()));
				}
			}
		}
	}
}


void
ClearPatch::post_process()
{
	if (_patch != NULL) {
		delete _ports_array;

		// Restore patch's run state
		if (_process)
			_patch->enable();
		else
			_patch->disable();

		// Make sure everything's sane
		assert(_patch->nodes().size() == 0);
		assert(_patch->num_ports() == 0);
		assert(_patch->connections().size() == 0);

		// Deactivate nodes
		for (EngineStore::Objects::iterator i = _removed_table->begin();
				i != _removed_table->end(); ++i) {
			SharedPtr<NodeImpl> node = PtrCast<NodeImpl>(i->second);
			if (node)
				node->deactivate();
		}

		// Unregister and destroy driver ports
		if (_driver_ports) {
			for (size_t i = 0; i < _driver_ports->size(); ++i) {
				Raul::List<DriverPort*>::Node* ln = _driver_ports->at(i);
				if (ln) {
					ln->elem()->destroy();
					_engine.maid()->push(ln);
				}
			}
			delete _driver_ports;
		}

		// Reply
		_responder->respond_ok();
		_engine.broadcaster()->send_clear_patch(_patch_path);

	} else {
		_responder->respond_error(string("Patch ") + _patch_path.str() + " not found");
	}

	_source->unblock(); // FIXME: can be done earlier in execute?
}


} // namespace Ingen
} // namespace Events
