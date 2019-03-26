/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_NODELIST_WINDOW_HPP
#define INGEN_GUI_NODELIST_WINDOW_HPP

#include <map>
#include <vector>
#include <algorithm>

#include <gtkmm/alignment.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/table.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treeview.h>

#include "ingen/client/BlockModel.hpp"
#include "ingen/types.hpp"
#include "ingen/Store.hpp"

#include "Window.hpp"

namespace ingen {

namespace client { class ObjectModel; }

namespace gui {

/** Object nodelist window.
 *
 * Loaded from XML as a derived object.
 *
 * \ingroup GUI
 */
class NodelistWindow : public Window
{
public:
	NodelistWindow(BaseObjectType*                   cobject,
                 const Glib::RefPtr<Gtk::Builder>& xml);

	void present(SPtr<const client::ObjectModel> model);
	void set_object(SPtr<const client::ObjectModel> model);

private:
	/** Record of a node (row in the table) */
	struct Record {
		Record(Gtk::Alignment* vw, int r, Gtk::CheckButton* cb)
			: row(r)
		{}
		int               row;
	};

	bool datatype_supported(const std::set<URI>& types,
	                        URI*                 widget_type);

	bool class_supported(const std::set<URI>& types);

	void on_show() override;

	void ok_clicked();

  class ModelColumns : public Gtk::TreeModel::ColumnRecord
  {
  public:
    ModelColumns()
    {
      add(_col_id);
      add(_col_name);
    }

    Gtk::TreeModelColumn<unsigned int>  _col_id;
    Gtk::TreeModelColumn<Glib::ustring> _col_name;
  };

	typedef std::map<URI, Record> Records;
	Records _records;

	SPtr<const client::ObjectModel> _model;
	Glib::RefPtr<Gtk::ListStore>    _node_store;
	Gtk::VBox*                      _vbox;
	Gtk::Button*                    _ok_button;
  Gtk::TreeView*                  _tree_view;
  ModelColumns                    _model_columns;
  std::vector<std::string>        _node_instances; 
  int                             _node_num = 1; 
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_NODELIST_WINDOW_HPP
