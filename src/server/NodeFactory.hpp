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

#ifndef INGEN_ENGINE_NODEFACTORY_HPP
#define INGEN_ENGINE_NODEFACTORY_HPP

#include <map>

#include "raul/SharedPtr.hpp"
#include "raul/URI.hpp"

#include "shared/World.hpp"

namespace Ingen {
namespace Server {

class NodeImpl;
class PatchImpl;
class PluginImpl;
class LV2Info;

/** Discovers and loads plugin libraries.
 *
 * \ingroup engine
 */
class NodeFactory
{
public:
	explicit NodeFactory(Ingen::Shared::World* world);
	~NodeFactory();

	void load_plugins();

	typedef std::map<Raul::URI, PluginImpl*> Plugins;
	const Plugins& plugins();

	PluginImpl* plugin(const Raul::URI& uri);

private:
	void load_lv2_plugins();
	void load_internal_plugins();

	Plugins               _plugins;
	Ingen::Shared::World* _world;
	bool                  _has_loaded;

	SharedPtr<LV2Info>    _lv2_info;

};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_NODEFACTORY_HPP