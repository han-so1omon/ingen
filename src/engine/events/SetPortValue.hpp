/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef SETPORTVALUEEVENT_H
#define SETPORTVALUEEVENT_H

#include "raul/Atom.hpp"
#include "QueuedEvent.hpp"
#include "types.hpp"

namespace Ingen {

class PortImpl;

namespace Events {


/** An event to change the value of a port.
 *
 * This event can either be queued or immediate, depending on the queued
 * parameter passed to the constructor.  It must be passed to the appropriate
 * place (ie queued event passed to the event queue and non-queued event
 * processed in the audio thread) or nasty things will happen.
 *
 * \ingroup engine
 */
class SetPortValue : public QueuedEvent
{
public:
	SetPortValue(Engine&              engine,
	             SharedPtr<Responder> responder,
	             bool                 queued,
	             SampleCount          timestamp,
	             const Raul::Path&    port_path,
	             const Raul::Atom&    value);

	SetPortValue(Engine&              engine,
	             SharedPtr<Responder> responder,
	             bool                 queued,
	             SampleCount          timestamp,
	             uint32_t             voice_num,
	             const Raul::Path&    port_path,
	             const Raul::Atom&    value);

	SetPortValue(Engine&              engine,
	             SharedPtr<Responder> responder,
	             SampleCount          timestamp,
	             PortImpl*            port,
	             const Raul::Atom&    value);

	~SetPortValue();

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	enum ErrorType {
		NO_ERROR,
		PORT_NOT_FOUND,
		NO_SPACE,
		ILLEGAL_VOICE,
		TYPE_MISMATCH
	};

	void apply(uint32_t start, uint32_t nframes);

	bool             _queued;
	bool             _omni;
	uint32_t         _voice_num;
	const Raul::Path _port_path;
	const Raul::Atom _value;
	PortImpl*        _port;
	ErrorType        _error;
};


} // namespace Ingen
} // namespace Events

#endif // SETPORTVALUEEVENT_H