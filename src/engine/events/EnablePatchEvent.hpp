/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef ENABLEPATCHEVENT_H
#define ENABLEPATCHEVENT_H

#include <string>
#include "QueuedEvent.hpp"

using std::string;

namespace Raul { template <typename T> class Array; }

namespace Ingen {

class PatchImpl;
class NodeImpl;
class CompiledPatch;


/** Enables a patch's DSP processing.
 *
 * \ingroup engine
 */
class EnablePatchEvent : public QueuedEvent
{
public:
	EnablePatchEvent(Engine&              engine,
	                 SharedPtr<Responder> responder,
	                 SampleCount          timestamp,
	                 const string&        patch_path,
	                 bool                 enable);
	
	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	string         _patch_path;
	PatchImpl*     _patch;
	CompiledPatch* _compiled_patch; // Patch's new process order
	bool           _enable;
};


} // namespace Ingen


#endif // ENABLEPATCHEVENT_H