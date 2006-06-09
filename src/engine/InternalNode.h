/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef INTERNALNODE_H
#define INTERNALNODE_H

#include <cstdlib>
#include "NodeBase.h"
#include "Plugin.h"

namespace Om {

class Patch;


/** Base class for Internal (builtin) nodes
 *
 * \ingroup engine
 */
class InternalNode : public NodeBase
{
public:
	InternalNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size)
	: NodeBase(path, poly, parent, srate, buffer_size),
	  m_is_added(false)
	{
		m_plugin.lib_path("/Om");
	}
	
	virtual ~InternalNode() {}

	virtual void deactivate() { if (m_is_added) remove_from_patch(); NodeBase::deactivate(); }
	
	virtual void run(size_t nframes) { NodeBase::run(nframes); }

	virtual void add_to_patch()      { assert(!m_is_added); m_is_added = true; }
	virtual void remove_from_patch() { assert(m_is_added); m_is_added = false; }

	//virtual void send_creation_messages(ClientInterface* client) const
	//{ NodeBase::send_creation_messages(client); }
	
	virtual const Plugin* plugin() const              { return &m_plugin; }
	virtual void          plugin(const Plugin* const) { exit(EXIT_FAILURE); }

protected:
	// Disallow copies (undefined)
	InternalNode(const InternalNode&);
	InternalNode& operator=(const InternalNode&);
	
	Plugin m_plugin;
	bool   m_is_added;
};


} // namespace Om

#endif // INTERNALNODE_H
