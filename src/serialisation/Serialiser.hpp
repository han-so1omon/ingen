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

#ifndef INGEN_SERIALISATION_SERIALISER_HPP
#define INGEN_SERIALISATION_SERIALISER_HPP

#include <cassert>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"

#include "sord/sordmm.hpp"

#include "interface/GraphObject.hpp"
#include "shared/Store.hpp"

namespace Ingen {

namespace Shared {
class Plugin;
class GraphObject;
class Patch;
class Node;
class Port;
class Connection;
class World;
}

namespace Serialisation {

/** Serialises Ingen objects (patches, nodes, etc) to RDF.
 *
 * \ingroup IngenClient
 */
class Serialiser
{
public:
	Serialiser(Shared::World& world, SharedPtr<Shared::Store> store);

	typedef Shared::GraphObject::Properties Properties;

	void to_file(SharedPtr<Shared::GraphObject> object,
	             const std::string&             filename);

	void write_bundle(SharedPtr<Shared::Patch> patch,
	                  const std::string&       uri);

	std::string to_string(SharedPtr<Shared::GraphObject> object,
	                      const std::string&             base_uri,
	                      const Properties&              extra_rdf);

	void start_to_string(const Raul::Path& root, const std::string& base_uri);
	void serialise(SharedPtr<Shared::GraphObject> object) throw (std::logic_error);
	void serialise_connection(const Sord::Node&             parent,
	                          SharedPtr<Shared::Connection> c) throw (std::logic_error);

	std::string finish();

private:
	enum Mode { TO_FILE, TO_STRING };

	void start_to_filename(const std::string& filename);

	void serialise_patch(SharedPtr<Shared::Patch> p,
	                     const Sord::Node&        id);

	void serialise_node(SharedPtr<Shared::Node> n,
	                    const Sord::Node&       class_id,
	                    const Sord::Node&       id);

	void serialise_port(const Shared::Port*     p,
	                    Shared::Resource::Graph context,
	                    const Sord::Node&       id);

	void serialise_properties(const Shared::GraphObject* o,
	                          Shared::Resource::Graph    context,
	                          Sord::Node                 id);

	Sord::Node path_rdf_node(const Raul::Path& path);

	void write_manifest(const std::string&       bundle_path,
	                    SharedPtr<Shared::Patch> patch,
	                    const std::string&       patch_symbol);

	Raul::Path               _root_path;
	SharedPtr<Shared::Store> _store;
	Mode                     _mode;
	std::string              _base_uri;
	Shared::World&           _world;
	Sord::Model*             _model;
};

} // namespace Serialisation
} // namespace Ingen

#endif // INGEN_SERIALISATION_SERIALISER_HPP
