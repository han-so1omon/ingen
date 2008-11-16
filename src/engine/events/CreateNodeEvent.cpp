/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include "raul/Path.hpp"
#include "raul/Path.hpp"
#include "redlandmm/World.hpp"
#include "module/World.hpp"
#include "CreateNodeEvent.hpp"
#include "Responder.hpp"
#include "PatchImpl.hpp"
#include "NodeImpl.hpp"
#include "PluginImpl.hpp"
#include "Engine.hpp"
#include "PatchImpl.hpp"
#include "NodeFactory.hpp"
#include "ClientBroadcaster.hpp"
#include "EngineStore.hpp"
#include "PortImpl.hpp"
#include "AudioDriver.hpp"

namespace Ingen {


CreateNodeEvent::CreateNodeEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path,
		const string& plugin_uri, bool polyphonic)
: QueuedEvent(engine, responder, timestamp),
  _path(path),
  _plugin_uri(plugin_uri),
  _polyphonic(polyphonic),
  _patch(NULL),
  _node(NULL),
  _compiled_patch(NULL),
  _node_already_exists(false)
{
}


/** DEPRECATED: Construct from type, library name, and plugin label.
 *
 * Do not use.
 */
CreateNodeEvent::CreateNodeEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path,
		const string& plugin_type, const string& plugin_lib, const string& plugin_label, bool polyphonic)
: QueuedEvent(engine, responder, timestamp),
  _path(path),
  _plugin_type(plugin_type),
  _plugin_lib(plugin_lib),
  _plugin_label(plugin_label),
  _polyphonic(polyphonic),
  _patch(NULL),
  _node(NULL),
  _compiled_patch(NULL),
  _node_already_exists(false)
{
}


void
CreateNodeEvent::pre_process()
{
	if (_engine.engine_store()->find_object(_path) != NULL) {
		_node_already_exists = true;
		QueuedEvent::pre_process();
		return;
	}

	_patch = _engine.engine_store()->find_patch(_path.parent());

	PluginImpl* const plugin = (_plugin_uri != "")
			? _engine.node_factory()->plugin(_plugin_uri)
			: _engine.node_factory()->plugin(_plugin_type, _plugin_lib, _plugin_label);

	if (_patch && plugin) {
			
		_node = plugin->instantiate(_path.name(), _polyphonic, _patch, _engine);
		
		if (_node != NULL) {
			_node->activate();
		
			// This can be done here because the audio thread doesn't touch the
			// node tree - just the process order array
			_patch->add_node(new PatchImpl::Nodes::Node(_node));
			//_node->add_to_store(_engine.engine_store());
			_engine.engine_store()->add(_node);
			
			// FIXME: not really necessary to build process order since it's not connected,
			// just append to the list
			if (_patch->enabled())
				_compiled_patch = _patch->compile();
		}
	}
	QueuedEvent::pre_process();
}


void
CreateNodeEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_node != NULL) {
		if (_patch->compiled_patch() != NULL)
			_engine.maid()->push(_patch->compiled_patch());
		_patch->compiled_patch(_compiled_patch);
	}
}


void
CreateNodeEvent::post_process()
{
	string msg;
	if (_node_already_exists) {
		msg = string("Could not create node - ").append(_path);// + " already exists.";
		_responder->respond_error(msg);
	} else if (_patch == NULL) {
		msg = "Could not find patch '" + _path.parent() +"' for add_node.";
		_responder->respond_error(msg);
	} else if (_node == NULL) {
		msg = "Unable to load node ";
		msg += _path + " (you're missing the plugin ";
		if (_plugin_uri != "")
			msg += _plugin_uri;
		else
			msg += _plugin_lib + ":" + _plugin_label + " (" + _plugin_type + ")";
		msg += ")";
		_responder->respond_error(msg);
	} else {
		_responder->respond_ok();
		_engine.broadcaster()->send_object(_node, true); // yes, send ports
	}
}


} // namespace Ingen

