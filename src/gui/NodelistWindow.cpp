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

#include <algorithm>
#include <cassert>
#include <set>

#include <gtkmm/label.h>
#include <gtkmm/spinbutton.h>

#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/World.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/PluginModel.hpp"

#include "App.hpp"
#include "NodelistWindow.hpp"
#include "RDFS.hpp"
#include "URIEntry.hpp"

namespace ingen {

using namespace client;

namespace gui {

typedef std::set<URI> URISet;

NodelistWindow::NodelistWindow(BaseObjectType*                   cobject,
                               const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
{
	xml->get_widget("nodelist_vbox", _vbox);
	xml->get_widget("nodelist_ok_button", _ok_button);
	xml->get_widget("nodelist_treeview", _tree_view);

  //TODO connect these to node list
  _node_store = Gtk::ListStore::create(_model_columns);
  _tree_view->set_model(_node_store);
  _tree_view->append_column("_ID", _model_columns._col_id);
  _tree_view->append_column("_Name", _model_columns._col_name);

	// This could be nicer.. store the TreeViewColumns locally maybe?
	_tree_view->get_column(0)->set_sort_column(_model_columns._col_id);
	_tree_view->get_column(1)->set_sort_column(_model_columns._col_name);
	for (int i = 0; i < 2; ++i) {
		_tree_view->get_column(i)->set_resizable(true);
	}

	_ok_button->signal_clicked().connect(
		sigc::mem_fun(this, &NodelistWindow::ok_clicked));
}

void
NodelistWindow::present(SPtr<const ObjectModel> model)
{
	set_object(model);
	Gtk::Window::present();
}

bool
NodelistWindow::datatype_supported(const rdfs::URISet& types,
                                   URI*                widget_type)
{
	if (types.find(_app->uris().atom_Int) != types.end()) {
		*widget_type = _app->uris().atom_Int;
		return true;
	} else if (types.find(_app->uris().atom_Float) != types.end()) {
		*widget_type = _app->uris().atom_Float;
		return true;
	} else if (types.find(_app->uris().atom_Bool) != types.end()) {
		*widget_type = _app->uris().atom_Bool;
		return true;
	} else if (types.find(_app->uris().atom_String) != types.end()) {
		*widget_type = _app->uris().atom_String;
		return true;
	} else if (types.find(_app->uris().atom_URID) != types.end()) {
		*widget_type = _app->uris().atom_URID;
		return true;
	}

	return false;
}

bool
NodelistWindow::class_supported(const rdfs::URISet& types)
{
	World*    world    = _app->world();
	LilvNode* rdf_type = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDF "type");

	for (const auto& t : types) {
		LilvNode*   range     = lilv_new_uri(world->lilv_world(), t.c_str());
		LilvNodes*  instances = lilv_world_find_nodes(
			world->lilv_world(), nullptr, rdf_type, range);

		const bool has_instance = (lilv_nodes_size(instances) > 0);

		lilv_nodes_free(instances);
		lilv_node_free(range);
		if (has_instance) {
			lilv_node_free(rdf_type);
			return true;
		}
	}

	lilv_node_free(rdf_type);
	return false;
}

/** Set the node this window is associated with.
 * This function MUST be called before using this object in any way.
 */
void
NodelistWindow::set_object(SPtr<const ObjectModel> model)
{
	_model = model;

	set_title(model->path() + " Nodelist - Ingen");

	World* world = _app->world();

	LilvNode* rdf_type = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDF "type");
	LilvNode* rdfs_DataType = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDFS "Datatype");

	// Populate key combo
	const URISet               props = rdfs::properties(world, model);
	std::map<std::string, URI> entries;
	for (const auto& p : props) {
		LilvNode*         prop   = lilv_new_uri(world->lilv_world(), p.c_str());
		const std::string label  = rdfs::label(world, prop);
		URISet            ranges = rdfs::range(world, prop, true);

		lilv_node_free(prop);
		if (label.empty() || ranges.empty()) {
			// Property has no label or range, can't show a widget for it
			continue;
		}

		LilvNode* range = lilv_new_uri(world->lilv_world(), (*ranges.begin()).c_str());
		if (rdfs::is_a(world, range, rdfs_DataType)) {
			// Range is a datatype, show if type or any subtype is supported
			rdfs::datatypes(_app->world(), ranges, false);
			URI widget_type("urn:nothing");
			if (datatype_supported(ranges, &widget_type)) {
				entries.emplace(label, p);
			}
		} else {
			// Range is presumably a class, show if any instances are known
			if (class_supported(ranges)) {
				entries.emplace(label, p);
			}
		}
	}

	lilv_node_free(rdfs_DataType);
	lilv_node_free(rdf_type);
}

void
NodelistWindow::on_show()
{
	static const int WIN_PAD  = 64;
	static const int VBOX_PAD = 16;

	int width  = 0;
	int height = 0;

  Gtk::TreeModel::Row row;

  //FIXME Update node vector when nodes are added/deleted,
  //      not here
  auto store = _app->world()->store();
  for (auto itr = store->begin(); itr != store->end(); itr++) {
    auto node_str = itr->second->symbol().c_str();
    auto search = std::find(
      _node_instances.begin(),
      _node_instances.end(),
      node_str
    );
    if (search == _node_instances.end()) {
      _node_instances.push_back(node_str);
      row = *(_node_store->append());
      row[_model_columns._col_id] = _node_num++;
      //row[_model_columns._col_name] = itr->second->base_uri().string();
      row[_model_columns._col_name] = node_str;
    }
  }

  //msg << ((boost::format(" (%1%)") % plugin->human_name()).str());

	for (const auto& c : _vbox->children()) {
		const Gtk::Requisition& req = c.get_widget()->size_request();

		width   = std::max(width, req.width);
		height += req.height + VBOX_PAD;
	}

	Gtk::Window::on_show();
}

void
NodelistWindow::ok_clicked()
{
	Gtk::Window::hide();
}

} // namespace gui
} // namespace ingen
