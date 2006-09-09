/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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


#ifndef SUBPATCHMODULE_H
#define SUBPATCHMODULE_H

#include <string>
#include <libgnomecanvasmm.h>
#include "OmModule.h"
#include "PatchController.h"
using std::string; using std::list;

namespace Ingen { namespace Client {
	class PatchModel;
	class NodeModel;
	class PortModel;
	class PatchWindow;
} }
using namespace Ingen::Client;

namespace Ingenuity {
	
class OmFlowCanvas;
class NodeControlWindow;
class PatchController;


/** A module to represent a subpatch
 *
 * \ingroup Ingenuity
 */
class SubpatchModule : public OmModule
{
public:
	SubpatchModule(OmFlowCanvas* canvas, PatchController* controller);
	virtual ~SubpatchModule() {}

	void on_double_click(GdkEventButton* ev);

	void show_dialog();
	void browse_to_patch();
	void menu_remove();

	PatchController* patch() { return m_patch; }

protected:
	PatchController* m_patch;
};


} // namespace Ingenuity

#endif // SUBPATCHMODULE_H
