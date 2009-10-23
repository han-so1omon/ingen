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

#include "events/RequestAllObjects.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "ObjectSender.hpp"
#include "ClientBroadcaster.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"

namespace Ingen {
namespace Events {


RequestAllObjects::RequestAllObjects(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp)
: QueuedEvent(engine, responder, timestamp)
{
}


void
RequestAllObjects::pre_process()
{
	QueuedEvent::pre_process();
}


void
RequestAllObjects::post_process()
{
	if (_responder->client()) {
		_responder->respond_ok();

		// Everything is a child of the root patch, so this sends it all
		PatchImpl* root = _engine.engine_store()->find_patch("/");
		if (root && _responder->client())
			ObjectSender::send_object(_responder->client(), root, true);

	} else {
		_responder->respond_error("Unable to find client to send all objects");
	}
}


} // namespace Ingen
} // namespace Events
