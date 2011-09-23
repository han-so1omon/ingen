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

#ifndef INGEN_ENGINE_POSTPROCESSOR_HPP
#define INGEN_ENGINE_POSTPROCESSOR_HPP

#include "raul/AtomicInt.hpp"
#include "raul/AtomicPtr.hpp"

#include "types.hpp"

namespace Ingen {
namespace Server {

class QueuedEvent;
class Engine;

/** Processor for Events after leaving the audio thread.
 *
 * The audio thread pushes events to this when it is done with them (which
 * is realtime-safe), which signals the processing thread through a semaphore
 * to handle the event and pass it on to the Maid.
 *
 * Update: This is all run from main_iteration now to solve scripting
 * thread issues.  Not sure if this is permanent/ideal or not...
 *
 * \ingroup engine
 */
class PostProcessor
{
public:
	PostProcessor(Engine& engine);
	~PostProcessor();

	/** Push a list of events on to the process queue.
	    realtime-safe, not thread-safe.
	*/
	void append(QueuedEvent* first, QueuedEvent* last);

	/** Post-process and delete all pending events */
	void process();

	/** Set the latest event time that should be post-processed */
	void set_end_time(FrameTime time) { _max_time = time; }

private:
	Engine&                      _engine;
	Raul::AtomicPtr<QueuedEvent> _head;
	Raul::AtomicPtr<QueuedEvent> _tail;
	Raul::AtomicInt              _max_time;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_POSTPROCESSOR_HPP