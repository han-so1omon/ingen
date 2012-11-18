/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>

#include "ingen/Log.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"

#include "BlockImpl.hpp"
#include "BufferFactory.hpp"
#include "DuplexPort.hpp"
#include "EdgeImpl.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "GraphPlugin.hpp"
#include "PortImpl.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

GraphImpl::GraphImpl(Engine&             engine,
                     const Raul::Symbol& symbol,
                     uint32_t            poly,
                     GraphImpl*          parent,
                     SampleRate          srate,
                     uint32_t            internal_poly)
	: BlockImpl(new GraphPlugin(engine.world()->uris(),
	                            engine.world()->uris().ingen_Graph,
	                            Raul::Symbol("graph"),
	                            "Ingen Graph"),
	           symbol, poly, parent, srate)
	, _engine(engine)
	, _poly_pre(internal_poly)
	, _poly_process(internal_poly)
	, _compiled_graph(NULL)
	, _process(false)
{
	assert(internal_poly >= 1);
	assert(internal_poly <= 128);
}

GraphImpl::~GraphImpl()
{
	delete _compiled_graph;
	delete _plugin;
}

void
GraphImpl::activate(BufferFactory& bufs)
{
	BlockImpl::activate(bufs);

	for (Blocks::iterator i = _blocks.begin(); i != _blocks.end(); ++i) {
		i->activate(bufs);
	}

	assert(_activated);
}

void
GraphImpl::deactivate()
{
	if (_activated) {
		BlockImpl::deactivate();

		for (Blocks::iterator i = _blocks.begin(); i != _blocks.end(); ++i) {
			if (i->activated()) {
				i->deactivate();
			}
		}
	}
}

void
GraphImpl::disable(ProcessContext& context)
{
	_process = false;
	for (Ports::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
		i->clear_buffers();
	}
}

bool
GraphImpl::prepare_internal_poly(BufferFactory& bufs, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	// TODO: Subgraph dynamic polyphony (i.e. changing port polyphony)

	for (Blocks::iterator i = _blocks.begin(); i != _blocks.end(); ++i) {
		i->prepare_poly(bufs, poly);
	}

	_poly_pre = poly;
	return true;
}

bool
GraphImpl::apply_internal_poly(ProcessContext& context,
                               BufferFactory&  bufs,
                               Raul::Maid&     maid,
                               uint32_t        poly)
{
	// TODO: Subgraph dynamic polyphony (i.e. changing port polyphony)

	for (Blocks::iterator i = _blocks.begin(); i != _blocks.end(); ++i) {
		i->apply_poly(context, maid, poly);
	}

	for (Blocks::iterator i = _blocks.begin(); i != _blocks.end(); ++i) {
		for (uint32_t j = 0; j < i->num_ports(); ++j) {
			PortImpl* const port = i->port_impl(j);
			if (port->is_input() && dynamic_cast<InputPort*>(port)->direct_connect())
				port->setup_buffers(bufs, port->poly(), true);
			port->connect_buffers();
		}
	}

	const bool polyphonic = parent_graph() && (poly == parent_graph()->internal_poly_process());
	for (Ports::iterator i = _outputs.begin(); i != _outputs.end(); ++i)
		i->setup_buffers(bufs, polyphonic ? poly : 1, true);

	_poly_process = poly;
	return true;
}

/** Run the graph for the specified number of frames.
 *
 * Calls all Blocks in (roughly, if parallel) the order _compiled_graph specifies.
 */
void
GraphImpl::process(ProcessContext& context)
{
	if (!_process)
		return;

	BlockImpl::pre_process(context);

	if (_compiled_graph && _compiled_graph->size() > 0) {
		// Run all blocks
		for (size_t i = 0; i < _compiled_graph->size(); ++i) {
			(*_compiled_graph)[i].block()->process(context);
		}
	}

	BlockImpl::post_process(context);
}

void
GraphImpl::set_buffer_size(Context&       context,
                           BufferFactory& bufs,
                           LV2_URID       type,
                           uint32_t       size)
{
	BlockImpl::set_buffer_size(context, bufs, type, size);

	for (size_t i = 0; i < _compiled_graph->size(); ++i)
		(*_compiled_graph)[i].block()->set_buffer_size(context, bufs, type, size);
}

// Graph specific stuff

/** Add a block.
 * Preprocessing thread only.
 */
void
GraphImpl::add_block(BlockImpl& block)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	_blocks.push_front(block);
}

/** Remove a block.
 * Preprocessing thread only.
 */
void
GraphImpl::remove_block(BlockImpl& block)
{
	_blocks.erase(_blocks.iterator_to(block));
}

void
GraphImpl::add_edge(SharedPtr<EdgeImpl> c)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	_edges.insert(make_pair(make_pair(c->tail(), c->head()), c));
}

/** Remove a edge.
 * Preprocessing thread only.
 */
SharedPtr<EdgeImpl>
GraphImpl::remove_edge(const PortImpl* tail, const PortImpl* dst_port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	Edges::iterator i = _edges.find(make_pair(tail, dst_port));
	if (i != _edges.end()) {
		SharedPtr<EdgeImpl> c = PtrCast<EdgeImpl>(i->second);
		_edges.erase(i);
		return c;
	} else {
		return SharedPtr<EdgeImpl>();
	}
}

bool
GraphImpl::has_edge(const PortImpl* tail, const PortImpl* dst_port) const
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	Edges::const_iterator i = _edges.find(make_pair(tail, dst_port));
	return (i != _edges.end());
}

uint32_t
GraphImpl::num_ports_non_rt() const
{
	ThreadManager::assert_not_thread(THREAD_PROCESS);
	return _inputs.size() + _outputs.size();
}

/** Create a port.  Not realtime safe.
 */
DuplexPort*
GraphImpl::create_port(BufferFactory&      bufs,
                       const Raul::Symbol& symbol,
                       PortType            type,
                       LV2_URID            buffer_type,
                       uint32_t            buffer_size,
                       bool                is_output,
                       bool                polyphonic)
{
	if (type == PortType::UNKNOWN) {
		bufs.engine().log().error(Raul::fmt("Unknown port type %1%\n")
		                          % type.uri());
		return NULL;
	}

	Raul::Atom value;
	if (type == PortType::CONTROL || type == PortType::CV)
		value = bufs.forge().make(0.0f);

	return new DuplexPort(bufs, this, symbol, num_ports_non_rt(), polyphonic, _polyphony,
	                      type, buffer_type, value, buffer_size, is_output);
}

/** Remove port from ports list used in pre-processing thread.
 *
 * Port is not removed from ports array for process thread (which could be
 * simultaneously running).
 *
 * Realtime safe.  Preprocessing thread only.
 */
void
GraphImpl::remove_port(DuplexPort& port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	if (port.is_input()) {
		_inputs.erase(_inputs.iterator_to(port));
	} else {
		_outputs.erase(_outputs.iterator_to(port));
	}
}

/** Remove all ports from ports list used in pre-processing thread.
 *
 * Ports are not removed from ports array for process thread (which could be
 * simultaneously running).  Returned is a (inputs, outputs) pair.
 *
 * Realtime safe.  Preprocessing thread only.
 */
void
GraphImpl::clear_ports()
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	_inputs.clear();
	_outputs.clear();
}

Raul::Array<PortImpl*>*
GraphImpl::build_ports_array()
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	const size_t n = _inputs.size() + _outputs.size();
	Raul::Array<PortImpl*>* const result = new Raul::Array<PortImpl*>(n);

	size_t i = 0;

	for (Ports::iterator p = _inputs.begin(); p != _inputs.end(); ++p, ++i)
		result->at(i) = &*p;

	for (Ports::iterator p = _outputs.begin(); p != _outputs.end(); ++p, ++i)
		result->at(i) = &*p;

	assert(i == n);

	return result;
}

static inline void
compile_recursive(BlockImpl* n, CompiledGraph* output)
{
	if (n == NULL || n->traversed())
		return;

	n->traversed(true);
	assert(output != NULL);

	for (std::list<BlockImpl*>::iterator i = n->providers().begin();
	     i != n->providers().end(); ++i)
		if (!(*i)->traversed())
			compile_recursive(*i, output);

	output->push_back(CompiledBlock(n, n->providers().size(), n->dependants()));
}

/** Find the process order for this Graph.
 *
 * The process order is a flat list that the graph will execute in order
 * when its run() method is called.  Return value is a newly allocated list
 * which the caller is reponsible to delete.  Note that this function does
 * NOT actually set the process order, it is returned so it can be inserted
 * at the beginning of an audio cycle (by various Events).
 *
 * Not realtime safe.
 */
CompiledGraph*
GraphImpl::compile()
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	CompiledGraph* const compiled_graph = new CompiledGraph();

	for (Blocks::iterator i = _blocks.begin(); i != _blocks.end(); ++i) {
		i->traversed(false);
	}

	for (Blocks::iterator i = _blocks.begin(); i != _blocks.end(); ++i) {
		// Either a sink or connected to our output ports:
		if (!i->traversed() && i->dependants().empty()) {
			compile_recursive(&*i, compiled_graph);
		}
	}

	// Traverse any blocks we didn't hit yet
	for (Blocks::iterator i = _blocks.begin(); i != _blocks.end(); ++i) {
		if (!i->traversed()) {
			compile_recursive(&*i, compiled_graph);
		}
	}

	if (compiled_graph->size() != _blocks.size()) {
		_engine.log().error(Raul::fmt("Failed to compile graph %1%\n") % _path);
		delete compiled_graph;
		return NULL;
	}

	return compiled_graph;
}

} // namespace Server
} // namespace Ingen