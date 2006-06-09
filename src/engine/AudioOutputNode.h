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

#ifndef AUDIOOUTPUTNODE_H
#define AUDIOOUTPUTNODE_H

#include <string>
#include "util/types.h"
#include "BridgeNode.h"

using std::string;

namespace Om {

template <typename T> class OutputPort;


/** Audio output BridgeNode.
 *
 * \ingroup engine
 */
class AudioOutputNode : public BridgeNode<sample>
{
public:
	AudioOutputNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size);
};


} // namespace Om

#endif // AUDIOOUTPUTNODE_H
