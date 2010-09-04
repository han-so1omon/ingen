/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_GUI_NEWSUBPATCHWINDOW_HPP
#define INGEN_GUI_NEWSUBPATCHWINDOW_HPP

#include <libglademm/xml.h>
#include <gtkmm.h>
#include "raul/SharedPtr.hpp"
#include "interface/GraphObject.hpp"
#include "Window.hpp"

namespace Ingen {

using namespace Shared;

namespace Client { class PatchModel; }
using Ingen::Client::PatchModel;

namespace GUI {


/** 'New Subpatch' window.
 *
 * Loaded by glade as a derived object.
 *
 * \ingroup GUI
 */
class NewSubpatchWindow : public Window
{
public:
	NewSubpatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void set_patch(SharedPtr<PatchModel> patch);

	void present(SharedPtr<PatchModel> patch, GraphObject::Properties data);

private:
	void name_changed();
	void ok_clicked();
	void cancel_clicked();

	GraphObject::Properties _initial_data;
	SharedPtr<PatchModel>  _patch;

	Gtk::Entry*      _name_entry;
	Gtk::Label*      _message_label;
	Gtk::SpinButton* _poly_spinbutton;
	Gtk::Button*     _ok_button;
	Gtk::Button*     _cancel_button;
};


} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_NEWSUBPATCHWINDOW_HPP
