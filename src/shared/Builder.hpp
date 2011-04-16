/* This file is part of Ingen.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_SHARED_BUILDER_HPP
#define INGEN_SHARED_BUILDER_HPP

#include "raul/SharedPtr.hpp"

namespace Raul { class Path; }

namespace Ingen {
namespace Shared {

class CommonInterface;
class GraphObject;
class LV2URIMap;

/** Wrapper for CommonInterface to create existing objects/models.
 *
 * \ingroup interface
 */
class Builder
{
public:
	Builder(SharedPtr<Shared::LV2URIMap> uris, CommonInterface& interface);
	virtual ~Builder() {}

	void build(SharedPtr<const GraphObject> object);
	void connect(SharedPtr<const GraphObject> object);

private:
	void build_object(SharedPtr<const GraphObject> object);

	SharedPtr<Shared::LV2URIMap> _uris;
	CommonInterface&             _interface;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_BUILDER_HPP

