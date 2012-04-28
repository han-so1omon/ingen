/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_CLIENT_THREADEDSIGCLIENTINTERFACE_HPP
#define INGEN_CLIENT_THREADEDSIGCLIENTINTERFACE_HPP

#include <stdint.h>

#include <string>

#undef nil
#include <sigc++/sigc++.h>
#include <glibmm/thread.h>

#include "raul/Atom.hpp"
#include "raul/SRSWQueue.hpp"

#include "ingen/Interface.hpp"
#include "ingen/client/SigClientInterface.hpp"

/** Returns nothing and takes no parameters (because they have all been bound) */
typedef sigc::slot<void> Closure;

namespace Ingen {

class Interface;

namespace Client {

/** A LibSigC++ signal emitting interface for clients to use.
 *
 * This emits signals (possibly) in a different thread than the ClientInterface
 * functions are called.  It must be explicitly driven with the emit_signals()
 * function, which fires all enqueued signals up until the present.  You can
 * use this in a GTK idle callback for receiving thread safe engine signals.
 */
class ThreadedSigClientInterface : public SigClientInterface
{
public:
	explicit ThreadedSigClientInterface(uint32_t queue_size)
		: _sigs(queue_size)
		, response_slot(_signal_response.make_slot())
		, error_slot(_signal_error.make_slot())
		, put_slot(_signal_put.make_slot())
		, connection_slot(_signal_connection.make_slot())
		, object_deleted_slot(_signal_object_deleted.make_slot())
		, object_moved_slot(_signal_object_moved.make_slot())
		, disconnection_slot(_signal_disconnection.make_slot())
		, disconnect_all_slot(_signal_disconnect_all.make_slot())
		, property_change_slot(_signal_property_change.make_slot())
	{}

	virtual Raul::URI uri() const { return "http://drobilla.net/ns/ingen#internal"; }

	void bundle_begin()
		{ push_sig(bundle_begin_slot); }

	void bundle_end()
		{ push_sig(bundle_end_slot); }

	void response(int32_t id, Status status)
		{ push_sig(sigc::bind(response_slot, id, status)); }

	void error(const std::string& msg)
		{ push_sig(sigc::bind(error_slot, msg)); }

	void put(const Raul::URI&            path,
	         const Resource::Properties& properties,
	         Resource::Graph             ctx=Resource::DEFAULT)
		{ push_sig(sigc::bind(put_slot, path, properties, ctx)); }

	void delta(const Raul::URI&            path,
	           const Resource::Properties& remove,
	           const Resource::Properties& add)
		{ push_sig(sigc::bind(delta_slot, path, remove, add)); }

	void connect(const Raul::Path& tail, const Raul::Path& head)
		{ push_sig(sigc::bind(connection_slot, tail, head)); }

	void del(const Raul::URI& uri)
		{ push_sig(sigc::bind(object_deleted_slot, uri)); }

	void move(const Raul::Path& old_path, const Raul::Path& new_path)
		{ push_sig(sigc::bind(object_moved_slot, old_path, new_path)); }

	void disconnect(const Raul::Path& tail, const Raul::Path& head)
		{ push_sig(sigc::bind(disconnection_slot, tail, head)); }

	void disconnect_all(const Raul::Path& parent_patch_path, const Raul::Path& path)
		{ push_sig(sigc::bind(disconnect_all_slot, parent_patch_path, path)); }

	void set_property(const Raul::URI& subject, const Raul::URI& key, const Raul::Atom& value)
		{ push_sig(sigc::bind(property_change_slot, subject, key, value)); }

	/** Process all queued events - Called from GTK thread to emit signals. */
	bool emit_signals();

private:
	void push_sig(Closure ev);

	Glib::Mutex _mutex;
	Glib::Cond  _cond;

	Raul::SRSWQueue<Closure> _sigs;

	sigc::slot<void>                                        bundle_begin_slot;
	sigc::slot<void>                                        bundle_end_slot;
	sigc::slot<void, int32_t, Status>                       response_slot;
	sigc::slot<void, std::string>                           error_slot;
	sigc::slot<void, Raul::URI, Raul::URI, Raul::Symbol>    new_plugin_slot;
	sigc::slot<void, Raul::URI, Resource::Properties,
	                            Resource::Graph>            put_slot;
	sigc::slot<void, Raul::URI, Resource::Properties,
	                            Resource::Properties>       delta_slot;
	sigc::slot<void, Raul::Path, Raul::Path>                connection_slot;
	sigc::slot<void, Raul::URI>                             object_deleted_slot;
	sigc::slot<void, Raul::Path, Raul::Path>                object_moved_slot;
	sigc::slot<void, Raul::Path, Raul::Path>                disconnection_slot;
	sigc::slot<void, Raul::Path, Raul::Path>                disconnect_all_slot;
	sigc::slot<void, Raul::URI, Raul::URI, Raul::Atom>      property_change_slot;
};

} // namespace Client
} // namespace Ingen

#endif
