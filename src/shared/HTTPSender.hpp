/* This file is part of Ingen.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_SHARED_HTTPSENDER_HPP
#define INGEN_SHARED_HTTPSENDER_HPP

#include <stdint.h>
#include <string>
#include <glibmm/thread.h>
#include "raul/Thread.hpp"

namespace Ingen {
namespace Shared {

class HTTPSender : public Raul::Thread {
public:
	HTTPSender();
	virtual ~HTTPSender();

	// Message bundling
	void bundle_begin();
	void bundle_end();

	// Transfers (loose bundling)
	void transfer_begin() { bundle_begin(); }
	void transfer_end()   { bundle_end(); }

	int listen_port() const { return _listen_port; }

protected:
	void _run();

	void send_chunk(const std::string& buf);

	enum SendState { Immediate, SendingBundle };

	Glib::Mutex _mutex;
	Glib::Cond  _signal;

	int          _listen_port;
	int          _listen_sock;
	int          _client_sock;
	SendState    _send_state;
	std::string  _transfer;
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_HTTPSENDER_HPP

