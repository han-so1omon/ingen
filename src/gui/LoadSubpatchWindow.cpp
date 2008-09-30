/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include <sys/types.h>
#include <dirent.h>
#include <cassert>
#include <boost/optional.hpp>
#include "interface/EngineInterface.hpp"
#include "client/NodeModel.hpp"
#include "client/PatchModel.hpp"
#include "App.hpp"
#include "LoadSubpatchWindow.hpp"
#include "PatchView.hpp"
#include "Configuration.hpp"
#include "ThreadedLoader.hpp"
using boost::optional;
using namespace std;

namespace Ingen {
namespace GUI {


LoadSubpatchWindow::LoadSubpatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::FileChooserDialog(cobject)
{
	xml->get_widget("load_subpatch_name_from_file_radio", _name_from_file_radio);
	xml->get_widget("load_subpatch_name_from_user_radio", _name_from_user_radio);
	xml->get_widget("load_subpatch_name_entry", _name_entry);
	xml->get_widget("load_subpatch_poly_from_file_radio", _poly_from_file_radio);
	xml->get_widget("load_subpatch_poly_from_parent_radio", _poly_from_parent_radio);
	xml->get_widget("load_subpatch_poly_from_user_radio", _poly_from_user_radio);
	xml->get_widget("load_subpatch_poly_spinbutton", _poly_spinbutton);
	xml->get_widget("load_subpatch_ok_button", _ok_button);
	xml->get_widget("load_subpatch_cancel_button", _cancel_button);

	_name_from_file_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadSubpatchWindow::disable_name_entry));
	_name_from_user_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadSubpatchWindow::enable_name_entry));
	_poly_from_file_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadSubpatchWindow::disable_poly_spinner));
	_poly_from_parent_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadSubpatchWindow::disable_poly_spinner));
	_poly_from_user_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadSubpatchWindow::enable_poly_spinner));
	_ok_button->signal_clicked().connect(sigc::mem_fun(this, &LoadSubpatchWindow::ok_clicked));
	_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &LoadSubpatchWindow::cancel_clicked));

	Gtk::FileFilter filt;
	filt.add_pattern("*.om");
	filt.set_name("Om patch files (XML, DEPRECATED) (*.om)");
	filt.add_pattern("*.ingen.ttl");
	filt.set_name("Ingen patch files (RDF, *.ingen.ttl)");
	set_filter(filt);

	property_select_multiple() = true;
	
	// Add global examples directory to "shortcut folders" (bookmarks)
	string examples_dir = INGEN_DATA_DIR;
	examples_dir.append("/patches");
	DIR* d = opendir(examples_dir.c_str());
	if (d != NULL)
		add_shortcut_folder(examples_dir);
}


void
LoadSubpatchWindow::present(SharedPtr<PatchModel> patch, GraphObject::Variables data)
{
	set_patch(patch);
	_initial_data = data;
	Gtk::Window::present();
}


/** Sets the patch controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
LoadSubpatchWindow::set_patch(SharedPtr<PatchModel> patch)
{
	_patch = patch;

	char temp_buf[4];
	snprintf(temp_buf, 4, "%u", patch->poly());
	Glib::ustring txt = "Same as parent (";
	txt.append(temp_buf).append(")");
	_poly_from_parent_radio->set_label(txt);
}


void
LoadSubpatchWindow::on_show()
{
	if (App::instance().configuration()->patch_folder().length() > 0)
		set_current_folder(App::instance().configuration()->patch_folder());
	Gtk::FileChooserDialog::on_show();
}


///// Event Handlers //////



void
LoadSubpatchWindow::disable_name_entry()
{
	_name_entry->property_sensitive() = false;
}


void
LoadSubpatchWindow::enable_name_entry()
{
	_name_entry->property_sensitive() = true;
}


void
LoadSubpatchWindow::disable_poly_spinner()
{
	_poly_spinbutton->property_sensitive() = false;
}


void
LoadSubpatchWindow::enable_poly_spinner()
{
	_poly_spinbutton->property_sensitive() = true;
}


void
LoadSubpatchWindow::ok_clicked()
{
	assert(_patch);
	
	// If unset load_patch will load values
	optional<Symbol> symbol;
	string name_str = "";
	
	if (_name_from_user_radio->get_active()) {
		name_str = _name_entry->get_text();
		symbol = Symbol::symbolify(name_str);
	}

	if (_poly_from_user_radio->get_active()) {
		cerr << "Overriding poly: " << _poly_spinbutton->get_value_as_int() << endl;
		_initial_data.insert(make_pair("ingen:polyphony", (int)_poly_spinbutton->get_value_as_int()));
	} else if (_poly_from_parent_radio->get_active()) {
		_initial_data.insert(make_pair("ingen:polyphony", (int)_patch->poly()));
	}

	std::list<Glib::ustring> uris = get_uris();
	for (std::list<Glib::ustring>::iterator i = uris.begin(); i != uris.end(); ++i) {
		// Cascade
		Atom& x = _initial_data["ingenuity:canvas-x"];
		x = Atom(x.get_float() + 20.0f);
		Atom& y = _initial_data["ingenuity:canvas-y"];
		y = Atom(y.get_float() + 20.0f);

		App::instance().loader()->load_patch(false, *i, "/", _initial_data, _patch->path(), symbol);
	}

	hide();
}			


void
LoadSubpatchWindow::cancel_clicked()
{
	hide();
}


} // namespace GUI
} // namespace Ingen