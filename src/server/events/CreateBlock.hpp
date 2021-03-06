/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_EVENTS_CREATEBLOCK_HPP
#define INGEN_EVENTS_CREATEBLOCK_HPP

#include "ingen/Resource.hpp"

#include "ClientUpdate.hpp"
#include "CompiledGraph.hpp"
#include "Event.hpp"

namespace ingen {
namespace server {

class BlockImpl;
class GraphImpl;

namespace events {

/** An event to load a Block and insert it into a Graph.
 *
 * \ingroup engine
 */
class CreateBlock : public Event
{
public:
	CreateBlock(Engine&           engine,
	            SPtr<Interface>   client,
	            int32_t           id,
	            SampleCount       timestamp,
	            const Raul::Path& path,
	            Properties&       properties);

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& context) override;
	void post_process() override;
	void undo(Interface& target) override;

private:
	Raul::Path          _path;
	Properties&         _properties;
	ClientUpdate        _update;
	GraphImpl*          _graph;
	BlockImpl*          _block;
	MPtr<CompiledGraph> _compiled_graph;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_CREATEBLOCK_HPP
