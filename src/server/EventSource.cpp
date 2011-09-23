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

#include "EventSource.hpp"
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"
#include "QueuedEvent.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

EventSource::EventSource()
	: _blocking_semaphore(0)
{
	Thread::set_context(THREAD_PRE_PROCESS);
	set_name("EventSource");
}

EventSource::~EventSource()
{
	Thread::stop();
}

/** Push an unprepared event onto the queue.
 */
void
EventSource::push_queued(QueuedEvent* const ev)
{
	assert(!ev->is_prepared());
	assert(!ev->next());

	QueuedEvent* const head = _head.get();
	QueuedEvent* const tail = _tail.get();

	if (!head) {
		_head = ev;
		_tail = ev;
	} else {
		_tail = ev;
		tail->next(ev);
	}

	if (!_prepared_back.get()) {
		_prepared_back = ev;
	}

	whip();
}

/** Process all events for a cycle.
 *
 * Executed events will be pushed to @a dest.
 */
void
EventSource::process(PostProcessor& dest, ProcessContext& context, bool limit)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	if (!_head.get())
		return;

	/* Limit the maximum number of queued events to process per cycle.  This
	   makes the process callback (more) realtime-safe by preventing being
	   choked by events coming in faster than they can be processed.
	   FIXME: test this and figure out a good value
	*/
	const size_t MAX_QUEUED_EVENTS = context.nframes() / 32;

	size_t num_events_processed = 0;

	QueuedEvent* ev   = _head.get();
	QueuedEvent* last = ev;

	while (ev && ev->is_prepared() && ev->time() < context.end()) {
		ev->execute(context);
		last = ev;
		ev = (QueuedEvent*)ev->next();
		++num_events_processed;
		if (limit && (num_events_processed > MAX_QUEUED_EVENTS))
			break;
	}

	if (num_events_processed > 0) {
		QueuedEvent* next = (QueuedEvent*)last->next();
		last->next(NULL);
		dest.append(_head.get(), last);
		_head = next;
		if (!next)
			_tail = NULL;
	}
}

/** Pre-process a single event */
void
EventSource::_whipped()
{
	QueuedEvent* ev = _prepared_back.get();
	if (!ev)
		return;

	assert(!ev->is_prepared());
	ev->pre_process();
	assert(ev->is_prepared());

	_prepared_back = (QueuedEvent*)ev->next();

	// If event was blocking, wait for event to being run through the
	// process thread before preparing the next event
	if (ev->is_blocking())
		_blocking_semaphore.wait();
}

} // namespace Server
} // namespace Ingen
