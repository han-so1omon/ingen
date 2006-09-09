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


#ifndef OMPORTMODULE_H
#define OMPORTMODULE_H

#include <string>
#include <libgnomecanvasmm.h>
#include <flowcanvas/Module.h>
#include "PortController.h"
using std::string;

namespace Ingen { namespace Client {
	class PortModel;
	class NodeModel;
	class ControlModel;
} }
using namespace Ingen::Client;

namespace Ingenuity {
	
class PatchController;
class PortController;
class OmFlowCanvas;
class OmPort;


/** A "module" to represent a patch's port on it's own canvas.
 *
 * Translation: This is the nameless single port pseudo module thingy.
 *
 * \ingroup Ingenuity
 */
class OmPortModule : public LibFlowCanvas::Module
{
public:
	OmPortModule(OmFlowCanvas* canvas, PortController* port, double x, double y);
	virtual ~OmPortModule() {}
	
	//virtual OmPort* port(const string& port_name) {
	//	return (OmPort*)Module::port(port_name);
	//}

	virtual void store_location();
	void move_to(double x, double y);

	//void on_right_click(GdkEventButton* event) { m_port->show_menu(event); }
	
	PortController* port() const { return m_port; }

protected:
	//virtual void on_double_click(GdkEventButton* ev) { show_control_window(); }
	//virtual void on_middle_click(GdkEventButton* ev) { show_control_window(); }
	
	PortController* m_port;
};


} // namespace Ingenuity

#endif // OMPORTMODULE_H
