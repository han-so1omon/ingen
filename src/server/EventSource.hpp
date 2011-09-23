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

#ifndef INGEN_ENGINE_EVENTSOURCE_HPP
#define INGEN_ENGINE_EVENTSOURCE_HPP

#include "raul/AtomicPtr.hpp"
#include "raul/Semaphore.hpp"
#include "raul/Slave.hpp"

namespace Ingen {
namespace Server {

class Event;
class QueuedEvent;
class PostProcessor;
class ProcessContext;

/** Source for events to run in the audio thread.
 *
 * The Driver gets events from an EventSource in the process callback
 * (realtime audio thread) and executes them, then they are sent to the
 * PostProcessor and finalised (post-processing thread).
 */
class EventSource : protected Raul::Slave
{
public:
	explicit EventSource();
	virtual ~EventSource();

	void process(PostProcessor& dest, ProcessContext& context, bool limit=true);

	bool empty() { return !_head.get(); }

	/** Signal that a blocking event is finished.
	 *
	 * This MUST be called by blocking events in their post_process() method
	 * to resume pre-processing of events.
	 */
	inline void unblock() { _blocking_semaphore.post(); }

protected:
	void push_queued(QueuedEvent* const ev);

	inline bool unprepared_events() { return (_prepared_back.get() != NULL); }

	virtual void _whipped(); ///< Prepare 1 event

private:
	Raul::AtomicPtr<QueuedEvent> _head;
	Raul::AtomicPtr<QueuedEvent> _prepared_back;
	Raul::AtomicPtr<QueuedEvent> _tail;

	Raul::Semaphore _blocking_semaphore;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_EVENTSOURCE_HPP
