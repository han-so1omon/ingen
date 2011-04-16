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

#ifndef INGEN_INTERFACE_PATCH_HPP
#define INGEN_INTERFACE_PATCH_HPP

#include <map>
#include <utility>

#include "raul/SharedPtr.hpp"

#include "ingen/Node.hpp"

namespace Ingen {
namespace Shared {

class Connection;

/** A Path (graph of Nodes/Connections)
 *
 * \ingroup interface
 */
class Patch : virtual public Node
{
public:
	typedef std::pair<const Port*, const Port*> ConnectionsKey;
	typedef std::map< ConnectionsKey, SharedPtr<Connection> > Connections;

	virtual const Connections& connections() const = 0;

	virtual bool     enabled()       const = 0;
	virtual uint32_t internal_poly() const = 0;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_INTERFACE_PATCH_HPP