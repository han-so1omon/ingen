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

#include <string>

#include "raul/log.hpp"

#include "Driver.hpp"
#include "Engine.hpp"
#include "EventSource.hpp"
#include "QueuedEngineInterface.hpp"
#include "events.hpp"

#define LOG(s) s << "[QueuedEngineInterface] "

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

QueuedEngineInterface::QueuedEngineInterface(Engine& engine)
	: EventSource()
	, _request(new Request(this, NULL, 0))
	, _engine(engine)
	, _in_bundle(false)
{
	start();
}


QueuedEngineInterface::~QueuedEngineInterface()
{
	stop();
}

SampleCount
QueuedEngineInterface::now() const
{
	// Exactly one cycle latency (some could run ASAP if we get lucky, but not always, and a slight
	// constant latency is far better than jittery lower (average) latency
	if (_engine.driver())
		return _engine.driver()->frame_time() + _engine.driver()->block_length();
	else
		return 0;
}

void
QueuedEngineInterface::set_next_response_id(int32_t id)
{
	if (_request)
		_request->set_id(id);
}

void
QueuedEngineInterface::disable_responses()
{
	_request->set_client(NULL);
	_request->set_id(0);
}

/* *** ServerInterface implementation below here *** */

void
QueuedEngineInterface::register_client(ClientInterface* client)
{
	push_queued(new Events::RegisterClient(_engine, _request, now(), client->uri(), client));
	if (!_request) {
		_request = SharedPtr<Request>(new Request(this, client, 1));
	} else {
		_request->set_id(1);
		_request->set_client(client);
	}
}

void
QueuedEngineInterface::unregister_client(const URI& uri)
{
	push_queued(new Events::UnregisterClient(_engine, _request, now(), uri));
	if (_request && _request->client() && _request->client()->uri() == uri) {
		_request->set_id(0);
		_request->set_client(NULL);
	}
}

// Bundle commands

void
QueuedEngineInterface::bundle_begin()
{
	_in_bundle = true;
}

void
QueuedEngineInterface::bundle_end()
{
	_in_bundle = false;
}

// Object commands

void
QueuedEngineInterface::put(const URI&                  uri,
                           const Resource::Properties& properties,
                           const Resource::Graph       ctx)
{
	push_queued(new Events::SetMetadata(_engine, _request, now(), true, ctx, uri, properties));
}

void
QueuedEngineInterface::delta(const URI&                  uri,
                             const Resource::Properties& remove,
                             const Resource::Properties& add)
{
	push_queued(new Events::SetMetadata(_engine, _request, now(), false, Resource::DEFAULT, uri, add, remove));
}

void
QueuedEngineInterface::move(const Path& old_path,
                            const Path& new_path)
{
	push_queued(new Events::Move(_engine, _request, now(), old_path, new_path));
}

void
QueuedEngineInterface::del(const URI& uri)
{
	if (uri == "ingen:engine") {
		_request->respond_ok();
		_engine.quit();
	} else {
		push_queued(new Events::Delete(_engine, _request, now(), uri));
	}
}

void
QueuedEngineInterface::connect(const Path& src_port_path,
                               const Path& dst_port_path)
{
	push_queued(new Events::Connect(_engine, _request, now(), src_port_path, dst_port_path));

}

void
QueuedEngineInterface::disconnect(const URI& src,
                                  const URI& dst)
{
	if (!Path::is_path(src) && !Path::is_path(dst)) {
		std::cerr << "Bad disconnect request " << src << " => " << dst << std::endl;
		return;
	}
		
	push_queued(new Events::Disconnect(_engine, _request, now(),
	                                   Path(src.str()), Path(dst.str())));
}

void
QueuedEngineInterface::disconnect_all(const Path& patch_path,
                                      const Path& path)
{
	push_queued(new Events::DisconnectAll(_engine, _request, now(), patch_path, path));
}

void
QueuedEngineInterface::set_property(const URI&  uri,
                                    const URI&  predicate,
                                    const Atom& value)
{
	if (uri == "ingen:engine" && predicate == "ingen:enabled"
	    && value.type() == Atom::BOOL) {
		if (value.get_bool()) {
			_engine.activate();
			push_queued(new Events::Ping(_engine, _request, now()));
		} else {
			push_queued(new Events::Deactivate(_engine, _request, now()));
		}
	} else {
		Resource::Properties remove;
		remove.insert(make_pair(predicate, _engine.world()->uris()->wildcard));
		Resource::Properties add;
		add.insert(make_pair(predicate, value));
		push_queued(new Events::SetMetadata(
			            _engine, _request, now(), false, Resource::DEFAULT,
			            uri, add, remove));
	}
}

// Requests //

void
QueuedEngineInterface::ping()
{
	push_queued(new Events::Ping(_engine, _request, now()));
}

void
QueuedEngineInterface::get(const URI& uri)
{
	push_queued(new Events::Get(_engine, _request, now(), uri));
}

void
QueuedEngineInterface::request_property(const URI& uri, const URI& key)
{
	push_queued(new Events::RequestMetadata(_engine, _request, now(), Resource::DEFAULT, uri, key));
}

} // namespace Server
} // namespace Ingen