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

#ifndef INGEN_GUI_SUBPATCHMODULE_HPP
#define INGEN_GUI_SUBPATCHMODULE_HPP

#include <string>
#include "raul/SharedPtr.hpp"
#include "PatchPortModule.hpp"
#include "NodeModule.hpp"
using std::string; using std::list;

namespace Ingen { namespace Client {
	class PatchModel;
	class NodeModel;
	class PortModel;
	class PatchWindow;
} }
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {

class PatchCanvas;
class NodeControlWindow;

/** A module to represent a subpatch
 *
 * \ingroup GUI
 */
class SubpatchModule : public NodeModule
{
public:
	SubpatchModule(PatchCanvas&                canvas,
	               SharedPtr<const PatchModel> controller);

	virtual ~SubpatchModule() {}

	void on_double_click(GdkEventButton* ev);

	void store_location(double x, double y);

	void browse_to_patch();
	void menu_remove();

	SharedPtr<const PatchModel> patch() const { return _patch; }

protected:
	SharedPtr<const PatchModel> _patch;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_SUBPATCHMODULE_HPP
