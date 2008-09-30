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

#include "interface/ClientInterface.hpp"
#include "Responder.hpp"
#include "UnregisterClientEvent.hpp"
#include "Engine.hpp"
#include "ClientBroadcaster.hpp"

namespace Ingen {


UnregisterClientEvent::UnregisterClientEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& uri)
: QueuedEvent(engine, responder, timestamp)
, _uri(uri)
{
}


void
UnregisterClientEvent::post_process()
{
	if (_engine.broadcaster()->unregister_client(_uri))
		_responder->respond_ok();
	else
		_responder->respond_error("Unable to unregister client");
}


} // namespace Ingen
