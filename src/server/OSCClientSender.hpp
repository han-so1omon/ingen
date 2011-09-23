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

#ifndef INGEN_ENGINE_OSCCLIENTSENDER_HPP
#define INGEN_ENGINE_OSCCLIENTSENDER_HPP

#include <cassert>
#include <string>

#include <lo/lo.h>

#include "ingen/ClientInterface.hpp"
#include "ingen/GraphObject.hpp"
#include "shared/OSCSender.hpp"

namespace Ingen {

class ServerInterface;

namespace Server {


/** Implements ClientInterface for OSC clients (sends OSC messages).
 *
 * \ingroup engine
 */
class OSCClientSender : public ClientInterface,
                        public Ingen::Shared::OSCSender
{
public:
	explicit OSCClientSender(const Raul::URI& url,
	                         size_t           max_packet_size)
		: Shared::OSCSender(max_packet_size)
		, _url(url)
	{
		_address = lo_address_new_from_url(url.c_str());
	}

	virtual ~OSCClientSender()
	{ lo_address_free(_address); }

	bool enabled() const { return _enabled; }

	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }

	void bundle_begin() { OSCSender::bundle_begin(); }
	void bundle_end()   { OSCSender::bundle_end(); }

	Raul::URI uri() const { return _url; }

	/* *** ClientInterface Implementation Below *** */

	void response_ok(int32_t id);
	void response_error(int32_t id, const std::string& msg);

	void error(const std::string& msg);

	virtual void put(const Raul::URI&            path,
	                 const Resource::Properties& properties,
	                 Resource::Graph             ctx=Resource::DEFAULT);

	virtual void delta(const Raul::URI&            path,
	                   const Resource::Properties& remove,
	                   const Resource::Properties& add);

	virtual void del(const Raul::URI& uri);

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path);

	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path);

	virtual void disconnect(const Raul::URI& src,
	                        const Raul::URI& dst);

	virtual void disconnect_all(const Raul::Path& parent_patch_path,
	                            const Raul::Path& path);

	virtual void set_property(const Raul::URI&  subject,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	virtual void activity(const Raul::Path& path,
	                      const Raul::Atom& value);

private:
	Raul::URI _url;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_OSCCLIENTSENDER_HPP
