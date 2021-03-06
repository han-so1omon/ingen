/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <boost/variant/apply_visitor.hpp>

#include "ingen/Log.hpp"
#include "ingen/client/ArcModel.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "ingen/client/SigClientInterface.hpp"

namespace ingen {
namespace client {

ClientStore::ClientStore(URIs&                    uris,
                         Log&                     log,
                         SPtr<SigClientInterface> emitter)
	: _uris(uris)
	, _log(log)
	, _emitter(emitter)
	, _plugins(new Plugins())
{
	if (emitter) {
		emitter->signal_message().connect(
			sigc::mem_fun(this, &ClientStore::message));
	}
}

void
ClientStore::clear()
{
	Store::clear();
	_plugins->clear();
}

void
ClientStore::add_object(SPtr<ObjectModel> object)
{
	// If we already have "this" object, merge the existing one into the new
	// one (with precedence to the new values).
	auto existing = find(object->path());
	if (existing != end()) {
		dynamic_ptr_cast<ObjectModel>(existing->second)->set(object);
	} else {
		if (!object->path().is_root()) {
			SPtr<ObjectModel> parent = _object(object->path().parent());
			if (parent) {
				assert(object->path().is_child_of(parent->path()));
				object->set_parent(parent);
				parent->add_child(object);
				assert(parent && (object->parent() == parent));

				(*this)[object->path()] = object;
				_signal_new_object.emit(object);
			} else {
				_log.error(fmt("Object %1% with no parent\n") % object->path());
			}
		} else {
			(*this)[object->path()] = object;
			_signal_new_object.emit(object);
		}
	}

	for (auto p : object->properties()) {
		object->signal_property().emit(p.first, p.second);
	}
}

SPtr<ObjectModel>
ClientStore::remove_object(const Raul::Path& path)
{
	// Find the object, the "top" of the tree to remove
	const iterator top = find(path);
	if (top == end()) {
		return SPtr<ObjectModel>();
	}

	SPtr<ObjectModel> object = dynamic_ptr_cast<ObjectModel>(top->second);

	// Remove object and any adjacent arcs from parent if applicable
	if (object && object->parent()) {
		SPtr<PortModel> port = dynamic_ptr_cast<PortModel>(object);
		if (port && dynamic_ptr_cast<GraphModel>(port->parent())) {
			disconnect_all(port->parent()->path(), path);
			if (port->parent()->parent()) {
				disconnect_all(port->parent()->parent()->path(), path);
			}
		} else if (port && port->parent()->parent()) {
			disconnect_all(port->parent()->parent()->path(), path);
		} else {
			disconnect_all(object->parent()->path(), path);
		}

		object->parent()->remove_child(object);
	}

	// Remove the object and all its descendants
	Objects removed;
	remove(top, removed);

	// Notify everything that needs to know this object has been removed
	if (object) {
		object->signal_destroyed().emit();
	}

	return object;
}

SPtr<PluginModel>
ClientStore::_plugin(const URI& uri)
{
	const Plugins::iterator i = _plugins->find(uri);
	return (i == _plugins->end()) ? SPtr<PluginModel>() : (*i).second;
}

SPtr<PluginModel>
ClientStore::_plugin(const Atom& uri)
{
	/* FIXME: Should probably be stored with URIs rather than strings, to make
	   this a fast case. */

	const Plugins::iterator i = _plugins->find(URI(_uris.forge.str(uri, false)));
	return (i == _plugins->end()) ? SPtr<PluginModel>() : (*i).second;
}

SPtr<const PluginModel>
ClientStore::plugin(const URI& uri) const
{
	return const_cast<ClientStore*>(this)->_plugin(uri);
}

SPtr<ObjectModel>
ClientStore::_object(const Raul::Path& path)
{
	const iterator i = find(path);
	if (i == end()) {
		return SPtr<ObjectModel>();
	} else {
		SPtr<ObjectModel> model = dynamic_ptr_cast<ObjectModel>(i->second);
		assert(model);
		assert(model->path().is_root() || model->parent());
		return model;
	}
}

SPtr<const ObjectModel>
ClientStore::object(const Raul::Path& path) const
{
	return const_cast<ClientStore*>(this)->_object(path);
}

SPtr<Resource>
ClientStore::_resource(const URI& uri)
{
	if (uri_is_path(uri)) {
		return _object(uri_to_path(uri));
	} else {
		return _plugin(uri);
	}
}

SPtr<const Resource>
ClientStore::resource(const URI& uri) const
{
	return const_cast<ClientStore*>(this)->_resource(uri);
}

void
ClientStore::add_plugin(SPtr<PluginModel> pm)
{
	SPtr<PluginModel> existing = _plugin(pm->uri());
	if (existing) {
		existing->set(pm);
	} else {
		_plugins->emplace(pm->uri(), pm);
		_signal_new_plugin.emit(pm);
	}
}

/* ****** Signal Handlers ******** */

void
ClientStore::operator()(const Del& del)
{
	if (uri_is_path(del.uri)) {
		remove_object(uri_to_path(del.uri));
	} else {
		auto p = _plugins->find(del.uri);
		if (p != _plugins->end()) {
			_plugins->erase(p);
			_signal_plugin_deleted.emit(del.uri);
		}
	}
}

void
ClientStore::operator()(const Copy&)
{
	_log.error("Client store copy unsupported\n");
}

void
ClientStore::operator()(const Move& msg)
{
	const iterator top = find(msg.old_path);
	if (top != end()) {
		rename(top, msg.new_path);
	}
}

void
ClientStore::message(const Message& msg)
{
	boost::apply_visitor(*this, msg);
}

void
ClientStore::operator()(const Put& msg)
{
	typedef Properties::const_iterator Iterator;

	const auto& uri        = msg.uri;
	const auto& properties = msg.properties;

	bool is_graph, is_block, is_port, is_output;
	Resource::type(uris(), properties,
	               is_graph, is_block, is_port, is_output);

	// Check for specially handled types
	const Iterator t = properties.find(_uris.rdf_type);
	if (t != properties.end()) {
		const Atom& type(t->second);
		if (_uris.pset_Preset == type) {
			const Iterator    p = properties.find(_uris.lv2_appliesTo);
			const Iterator    l = properties.find(_uris.rdfs_label);
			SPtr<PluginModel> plug;
			if (p == properties.end()) {
				_log.error(fmt("Preset <%1%> with no plugin\n") % uri.c_str());
			} else if (l == properties.end()) {
				_log.error(fmt("Preset <%1%> with no label\n") % uri.c_str());
			} else if (l->second.type() != _uris.forge.String) {
				_log.error(fmt("Preset <%1%> label is not a string\n") % uri.c_str());
			} else if (!(plug = _plugin(p->second))) {
				_log.error(fmt("Preset <%1%> for unknown plugin %2%\n")
				           % uri.c_str() % _uris.forge.str(p->second, true));
			} else {
				plug->add_preset(uri, l->second.ptr<char>());
			}
			return;
		} else if (_uris.ingen_Graph == type) {
			is_graph = true;
		} else if (_uris.ingen_Internal == type || _uris.lv2_Plugin == type) {
			SPtr<PluginModel> p(new PluginModel(uris(), uri, type, properties));
			add_plugin(p);
			return;
		}
	}

	if (!uri_is_path(uri)) {
		_log.error(fmt("Put for unknown subject <%1%>\n")
		           % uri.c_str());
		return;
	}

	const Raul::Path path(uri_to_path(uri));

	SPtr<ObjectModel> obj = dynamic_ptr_cast<ObjectModel>(_object(path));
	if (obj) {
		obj->set_properties(properties);
		return;
	}

	if (path.is_root()) {
		is_graph = true;
	}

	if (is_graph) {
		SPtr<GraphModel> model(new GraphModel(uris(), path));
		model->set_properties(properties);
		add_object(model);
	} else if (is_block) {
		auto p = properties.find(_uris.lv2_prototype);
		if (p == properties.end()) {
			p = properties.find(_uris.ingen_prototype);
		}

		SPtr<PluginModel> plug;
		if (p->second.is_valid() && (p->second.type() == _uris.forge.URI ||
		                             p->second.type() == _uris.forge.URID)) {
			const URI uri(_uris.forge.str(p->second, false));
			if (!(plug = _plugin(uri))) {
				plug = SPtr<PluginModel>(
					new PluginModel(uris(), uri, Atom(), Properties()));
				add_plugin(plug);
			}

			SPtr<BlockModel> bm(new BlockModel(uris(), plug, path));
			bm->set_properties(properties);
			add_object(bm);
		} else {
			_log.warn(fmt("Block %1% has no prototype\n") % path.c_str());
		}
	} else if (is_port) {
		PortModel::Direction pdir = (is_output)
			? PortModel::Direction::OUTPUT
			: PortModel::Direction::INPUT;
		uint32_t       index = 0;
		const Iterator i     = properties.find(_uris.lv2_index);
		if (i != properties.end() && i->second.type() == _uris.forge.Int) {
			index = i->second.get<int32_t>();
		}

		SPtr<PortModel> p(new PortModel(uris(), path, index, pdir));
		p->set_properties(properties);
		add_object(p);
	} else {
		_log.warn(fmt("Ignoring %1% of unknown type\n") % path.c_str());
	}
}

void
ClientStore::operator()(const Delta& msg)
{
	const auto& uri = msg.uri;
	if (uri == URI("ingen:/clients/this")) {
		// Client property, which we don't store (yet?)
		return;
	}

	if (!uri_is_path(uri)) {
		_log.error(fmt("Delta for unknown subject <%1%>\n")
		           % uri.c_str());
		return;
	}

	const Raul::Path path(uri_to_path(uri));

	SPtr<ObjectModel> obj = _object(path);
	if (obj) {
		obj->remove_properties(msg.remove);
		obj->add_properties(msg.add);
	} else {
		_log.warn(fmt("Failed to find object `%1%'\n") % path.c_str());
	}
}

void
ClientStore::operator()(const SetProperty& msg)
{
	const auto& subject_uri = msg.subject;
	const auto& predicate   = msg.predicate;
	const auto& value       = msg.value;

	if (subject_uri == URI("ingen:/engine")) {
		_log.info(fmt("Engine property <%1%> = %2%\n")
		          % predicate.c_str() % _uris.forge.str(value, false));
		return;
	}
	SPtr<Resource> subject = _resource(subject_uri);
	if (subject) {
		if (predicate == _uris.ingen_activity) {
			/* Activity is transient, trigger any live actions (like GUI
			   blinkenlights) but do not store the property. */
			subject->on_property(predicate, value);
		} else {
			subject->set_property(predicate, value, msg.ctx);
		}
	} else {
		SPtr<PluginModel> plugin = _plugin(subject_uri);
		if (plugin) {
			plugin->set_property(predicate, value);
		} else if (predicate != _uris.ingen_activity) {
			_log.warn(fmt("Property <%1%> for unknown object %2%\n")
			          % predicate.c_str() % subject_uri.c_str());
		}
	}
}

SPtr<GraphModel>
ClientStore::connection_graph(const Raul::Path& tail_path,
                              const Raul::Path& head_path)
{
	SPtr<GraphModel> graph;

	if (tail_path.parent() == head_path.parent()) {
		graph = dynamic_ptr_cast<GraphModel>(_object(tail_path.parent()));
	}

	if (!graph && tail_path.parent() == head_path.parent().parent()) {
		graph = dynamic_ptr_cast<GraphModel>(_object(tail_path.parent()));
	}

	if (!graph && tail_path.parent().parent() == head_path.parent()) {
		graph = dynamic_ptr_cast<GraphModel>(_object(head_path.parent()));
	}

	if (!graph) {
		graph = dynamic_ptr_cast<GraphModel>(_object(tail_path.parent().parent()));
	}

	if (!graph) {
		_log.error(fmt("Unable to find graph for arc %1% => %2%\n")
		           % tail_path % head_path);
	}

	return graph;
}

bool
ClientStore::attempt_connection(const Raul::Path& tail_path,
                                const Raul::Path& head_path)
{
	SPtr<PortModel> tail = dynamic_ptr_cast<PortModel>(_object(tail_path));
	SPtr<PortModel> head = dynamic_ptr_cast<PortModel>(_object(head_path));

	if (tail && head) {
		SPtr<GraphModel> graph = connection_graph(tail_path, head_path);
		SPtr<ArcModel>   arc(new ArcModel(tail, head));
		graph->add_arc(arc);
		return true;
	} else {
		_log.warn(fmt("Failed to connect %1% => %2%\n")
		          % tail_path % head_path);
		return false;
	}
}

void
ClientStore::operator()(const Connect& msg)
{
	attempt_connection(msg.tail, msg.head);
}

void
ClientStore::operator()(const Disconnect& msg)
{
	SPtr<PortModel>  tail  = dynamic_ptr_cast<PortModel>(_object(msg.tail));
	SPtr<PortModel>  head  = dynamic_ptr_cast<PortModel>(_object(msg.head));
	SPtr<GraphModel> graph = connection_graph(msg.tail, msg.head);
	if (graph) {
		graph->remove_arc(tail.get(), head.get());
	}
}

void
ClientStore::operator()(const DisconnectAll& msg)
{
	SPtr<GraphModel>  graph  = dynamic_ptr_cast<GraphModel>(_object(msg.graph));
	SPtr<ObjectModel> object = _object(msg.path);

	if (!graph || !object) {
		_log.error(fmt("Bad disconnect all notification %1% in %2%\n")
		           % msg.path % msg.graph);
		return;
	}

	const GraphModel::Arcs arcs = graph->arcs();
	for (auto a : arcs) {
		SPtr<ArcModel> arc = dynamic_ptr_cast<ArcModel>(a.second);
		if (arc->tail()->parent() == object
		    || arc->head()->parent() == object
		    || arc->tail()->path() == msg.path
		    || arc->head()->path() == msg.path) {
			graph->remove_arc(arc->tail().get(), arc->head().get());
		}
	}
}

} // namespace client
} // namespace ingen
