/* This file is part of Ingen.
 * Copyright 2008-2011 David Robillard <http://drobilla.net>
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

#include <algorithm>
#include "lv2/lv2plug.in/ns/ext/contexts/contexts.h"
#include "raul/log.hpp"
#include "ConnectionImpl.hpp"
#include "Engine.hpp"
#include "MessageContext.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

void
MessageContext::run(NodeImpl* node, FrameTime time)
{
	if (ThreadManager::thread_is(THREAD_PROCESS)) {
		const Request r(time, node);
		_requests.write(sizeof(Request), &r);
		// signal() will be called at the end of this process cycle
	} else {
		assert(node);
		Glib::Mutex::Lock lock(_mutex);
		_non_rt_request = Request(time, node);
		_sem.post();
		_cond.wait(_mutex);
	}
}

void
MessageContext::_run()
{
	Request req;

	while (true) {
		_sem.wait();

		// Enqueue a request from the pre-process thread
		{
			Glib::Mutex::Lock lock(_mutex);
			const Request req = _non_rt_request;
			if (req.node) {
				_queue.insert(req);
				_end_time = std::max(_end_time, req.time);
				_cond.broadcast(); // Notify caller we got the message
			}
		}

		// Enqueue (and thereby sort) requests from audio thread
		while (has_requests()) {
			_requests.read(sizeof(Request), &req);
			if (req.node) {
				_queue.insert(req);
			} else {
				_end_time = req.time;
				break;
			}
		}

		// Run events in time increasing order
		// Note that executing nodes may insert further events into the queue
		while (!_queue.empty()) {
			const Request req = *_queue.begin();

			// Break if all events during this cycle have been consumed
			// (the queue may contain generated events with later times)
			if (req.time > _end_time) {
				break;
			}

			_queue.erase(_queue.begin());
			execute(req);
		}
	}
}

void
MessageContext::execute(const Request& req)
{
	NodeImpl* node = req.node;
	node->message_run(*this);

	void*      valid_ports = node->valid_ports();
	PatchImpl* patch       = node->parent_patch();

	for (uint32_t i = 0; i < node->num_ports(); ++i) {
		PortImpl* p = node->port_impl(i);
		if (p->is_output() && p->context() == Context::MESSAGE &&
				lv2_contexts_port_is_valid(valid_ports, i)) {
			PatchImpl::Connections& wires = patch->connections();
			for (PatchImpl::Connections::iterator c = wires.begin(); c != wires.end(); ++c) {
				ConnectionImpl* ci = (ConnectionImpl*)c->second.get();
				if (ci->src_port() == p && ci->dst_port()->context() == Context::MESSAGE) {
					_queue.insert(Request(req.time, ci->dst_port()->parent_node()));
				}
			}
		}
	}

	node->reset_valid_ports();
}

} // namespace Server
} // namespace Ingen