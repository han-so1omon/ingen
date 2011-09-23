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

#ifndef INGEN_ENGINE_QUEUEDEVENT_HPP
#define INGEN_ENGINE_QUEUEDEVENT_HPP

#include "Event.hpp"

namespace Ingen {
namespace Server {

/** An Event with a not-time-critical preprocessing stage.
 *
 * These events are events that aren't able to be executed immediately by the
 * Jack thread (because they allocate memory or whatever).  They are pushed
 * on to the QueuedEventQueue where they are preprocessed then pushed on
 * to the realtime Event Queue when they are ready.
 *
 * Lookups for these events should go in the pre_process() method, since they are
 * not time critical and shouldn't waste time in the audio thread doing
 * lookups they can do beforehand.  (This applies for any expensive operation that
 * could be done before the execute() method).
 *
 * \ingroup engine
 */
class QueuedEvent : public Event
{
public:
	/** Process this event into a realtime-suitable event. */
	virtual void pre_process();

	virtual void execute(ProcessContext& context);

	/** True iff this event blocks the prepare phase of other events. */
	bool is_blocking() { return _blocking; }

	bool is_prepared() { return _pre_processed; }

protected:
	QueuedEvent(Engine&            engine,
	            SharedPtr<Request> request,
	            FrameTime          time,
	            bool               blocking=false)
		: Event(engine, request, time)
		, _pre_processed(false)
		, _blocking(blocking)
	{}

	// NULL event base (for internal events only!)
	explicit QueuedEvent(Engine& engine)
		: Event(engine, SharedPtr<Request>(), 0)
		, _pre_processed(false)
		, _blocking(false)
	{}

	bool _pre_processed;
	bool _blocking;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_QUEUEDEVENT_HPP