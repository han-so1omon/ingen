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

#ifndef INGEN_GUI_PORT_HPP
#define INGEN_GUI_PORT_HPP

#include <cassert>
#include <string>
#include "ganv/Port.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/WeakPtr.hpp"

namespace Raul {
class Atom;
class URI;
}

namespace Ingen {

namespace Client { class PortModel; }

namespace GUI {

class App;
class PatchBox;

/** A Port on an Module.
 *
 * \ingroup GUI
 */
class Port : public Ganv::Port
{
public:
	static Port* create(
		App&                               app,
		Ganv::Module&                      module,
		SharedPtr<const Client::PortModel> pm,
		bool                               human_name,
		bool                               flip = false);

	~Port();

	SharedPtr<const Client::PortModel> model() const { return _port_model.lock(); }

	bool show_menu(GdkEventButton* ev);
	void update_metadata();

	void value_changed(const Raul::Atom& value);
	void activity(const Raul::Atom& value);

	void set_selected(gboolean b);

private:
	Port(App&                               app,
	     Ganv::Module&                      module,
	     SharedPtr<const Client::PortModel> pm,
	     const std::string&                 name,
	     bool                               flip = false);

	PatchBox* get_patch_box() const;

	void property_changed(const Raul::URI& key, const Raul::Atom& value);
	void moved();

	void on_value_changed(GVariant* value);
	bool on_event(GdkEvent* ev);

	App&                             _app;
	WeakPtr<const Client::PortModel> _port_model;
	bool                             _pressed : 1;
	bool                             _flipped : 1;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PORT_HPP
