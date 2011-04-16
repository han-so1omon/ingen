/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENTS_CREATENODE_HPP
#define INGEN_EVENTS_CREATENODE_HPP

#include <string>
#include "QueuedEvent.hpp"
#include "ingen/Resource.hpp"

namespace Ingen {

class PatchImpl;
class PluginImpl;
class NodeImpl;
class CompiledPatch;

namespace Events {

/** An event to load a Node and insert it into a Patch.
 *
 * \ingroup engine
 */
class CreateNode : public QueuedEvent
{
public:
	CreateNode(
			Engine&                             engine,
			SharedPtr<Request>                  request,
			SampleCount                         timestamp,
			const Raul::Path&                   node_path,
			const Raul::URI&                    plugin_uri,
			const Shared::Resource::Properties& properties);

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	Raul::Path     _path;
	Raul::URI      _plugin_uri;
	PatchImpl*     _patch;
	PluginImpl*    _plugin;
	NodeImpl*      _node;
	CompiledPatch* _compiled_patch; ///< Patch's new process order
	bool           _node_already_exists;
	bool           _polyphonic;

	Shared::Resource::Properties _properties;
};

} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_CREATENODE_HPP
