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

#include <math.h>
#include "raul/midi_events.h"
#include "shared/LV2URIMap.hpp"
#include "internals/Controller.hpp"

#include "AudioBuffer.hpp"
#include "EventBuffer.hpp"
#include "InputPort.hpp"
#include "InternalPlugin.hpp"
#include "Notification.hpp"
#include "OutputPort.hpp"
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"
#include "util.hpp"

using namespace std;

namespace Ingen {
namespace Server {
namespace Internals {

InternalPlugin* ControllerNode::internal_plugin(Shared::LV2URIMap& uris) {
	return new InternalPlugin(uris, NS_INTERNALS "Controller", "controller");
}

ControllerNode::ControllerNode(InternalPlugin* plugin,
                               BufferFactory&  bufs,
                               const string&   path,
                               bool            polyphonic,
                               PatchImpl*      parent,
                               SampleRate      srate)
	: NodeImpl(plugin, path, false, parent, srate)
	, _learning(false)
{
	const Ingen::Shared::LV2URIMap& uris = bufs.uris();
	_ports = new Raul::Array<PortImpl*>(6);

	_midi_in_port = new InputPort(bufs, this, "input", 0, 1, PortType::EVENTS, Raul::Atom());
	_midi_in_port->set_property(uris.lv2_name, "Input");
	_ports->at(0) = _midi_in_port;

	_param_port = new InputPort(bufs, this, "controller", 1, 1, PortType::CONTROL, 0.0f);
	_param_port->set_property(uris.lv2_minimum, 0.0f);
	_param_port->set_property(uris.lv2_maximum, 127.0f);
	_param_port->set_property(uris.lv2_integer, true);
	_param_port->set_property(uris.lv2_name, "Controller");
	_ports->at(1) = _param_port;

	_log_port = new InputPort(bufs, this, "logarithmic", 2, 1, PortType::CONTROL, 0.0f);
	_log_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_log_port->set_property(uris.lv2_name, "Logarithmic");
	_ports->at(2) = _log_port;

	_min_port = new InputPort(bufs, this, "minimum", 3, 1, PortType::CONTROL, 0.0f);
	_min_port->set_property(uris.lv2_name, "Minimum");
	_ports->at(3) = _min_port;

	_max_port = new InputPort(bufs, this, "maximum", 4, 1, PortType::CONTROL, 1.0f);
	_max_port->set_property(uris.lv2_name, "Maximum");
	_ports->at(4) = _max_port;

	_audio_port = new OutputPort(bufs, this, "ar_output", 5, 1, PortType::AUDIO, 0.0f);
	_audio_port->set_property(uris.lv2_name, "Output");
	_ports->at(5) = _audio_port;
}

void
ControllerNode::process(ProcessContext& context)
{
	NodeImpl::pre_process(context);

	uint32_t frames    = 0;
	uint32_t subframes = 0;
	uint16_t type      = 0;
	uint16_t size      = 0;
	uint8_t* buf       = NULL;

	EventBuffer* const midi_in = (EventBuffer*)_midi_in_port->buffer(0).get();

	midi_in->rewind();

	while (midi_in->get_event(&frames, &subframes, &type, &size, &buf)) {
		// FIXME: type
		if (size >= 3 && (buf[0] & 0xF0) == MIDI_CMD_CONTROL)
			control(context, buf[1], buf[2], frames + context.start());

		midi_in->increment();
	}

	NodeImpl::post_process(context);
}

void
ControllerNode::control(ProcessContext& context, uint8_t control_num, uint8_t val, FrameTime time)
{
	Sample scaled_value;

	const Sample nval = (val / 127.0f); // normalized [0, 1]

	if (_learning) {
		_param_port->set_value(control_num);
		((AudioBuffer*)_param_port->buffer(0).get())->set_value(
				(float)control_num, context.start(), context.end());
		_param_port->broadcast_value(context, true);
		_learning = false;
	}

	const Sample min_port_val = ((AudioBuffer*)_min_port->buffer(0).get())->value_at(0);
	const Sample max_port_val = ((AudioBuffer*)_max_port->buffer(0).get())->value_at(0);
	const Sample log_port_val = ((AudioBuffer*)_log_port->buffer(0).get())->value_at(0);

	if (log_port_val > 0.0f) {
		// haaaaack, stupid negatives and logarithms
		Sample log_offset = 0;
		if (min_port_val < 0)
			log_offset = fabs(min_port_val);
		const Sample min = log(min_port_val + 1 + log_offset);
		const Sample max = log(max_port_val + 1 + log_offset);
		scaled_value = expf(nval * (max - min) + min) - 1 - log_offset;
	} else {
		scaled_value = ((nval) * (max_port_val - min_port_val)) + min_port_val;
	}

	if (control_num == ((AudioBuffer*)_param_port->buffer(0).get())->value_at(0))
		((AudioBuffer*)_audio_port->buffer(0).get())->set_value(scaled_value, context.start(), time);
}

} // namespace Internals
} // namespace Server
} // namespace Ingen
