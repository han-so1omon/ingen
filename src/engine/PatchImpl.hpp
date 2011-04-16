/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_PATCHIMPL_HPP
#define INGEN_ENGINE_PATCHIMPL_HPP

#include <cstdlib>
#include <string>
#include "raul/List.hpp"
#include "ingen/PortType.hpp"
#include "ingen/Patch.hpp"
#include "NodeImpl.hpp"
#include "PluginImpl.hpp"
#include "CompiledPatch.hpp"

namespace Ingen {

namespace Shared { class Connection; }

class CompiledPatch;
class ConnectionImpl;
class Context;
class Engine;
class ProcessContext;

/** A group of nodes in a graph, possibly polyphonic.
 *
 * Note that this is also a Node, just one which contains Nodes.
 * Therefore infinite subpatching is possible, of polyphonic
 * patches of polyphonic nodes etc. etc.
 *
 * \ingroup engine
 */
class PatchImpl : public NodeImpl, public Ingen::Shared::Patch
{
public:
	PatchImpl(Engine&             engine,
	          const Raul::Symbol& symbol,
	          uint32_t            poly,
	          PatchImpl*          parent,
	          SampleRate          srate,
	          uint32_t            local_poly);

	virtual ~PatchImpl();

	void activate(BufferFactory& bufs);
	void deactivate();

	void process(ProcessContext& context);

	void set_buffer_size(Context& context, BufferFactory& bufs, Shared::PortType type, size_t size);

	/** Prepare for a new (internal) polyphony value.
	 *
	 * Preprocessor thread, poly is actually applied by apply_internal_poly.
	 * \return true on success.
	 */
	bool prepare_internal_poly(BufferFactory& bufs, uint32_t poly);

	/** Apply a new (internal) polyphony value.
	 *
	 * Audio thread.
	 *
	 * \param context Process context
	 * \param bufs New set of buffers
	 * \param poly Must be < the most recent value passed to prepare_internal_poly.
	 * \param maid Any objects no longer needed will be pushed to this
	 */
	bool apply_internal_poly(ProcessContext& context, BufferFactory& bufs, Raul::Maid& maid, uint32_t poly);

	// Patch specific stuff not inherited from Node

	typedef Raul::List<NodeImpl*> Nodes;

	void         add_node(Nodes::Node* tn);
	Nodes::Node* remove_node(const Raul::Symbol& symbol);

	Nodes&       nodes()       { return _nodes; }
	Connections& connections() { return _connections; }

	const Nodes&       nodes()       const { return _nodes; }
	const Connections& connections() const { return _connections; }

	uint32_t num_ports() const;

	PortImpl* create_port(BufferFactory& bufs, const std::string& name, Shared::PortType type, size_t buffer_size, bool is_output, bool polyphonic);
	void add_input(Raul::List<PortImpl*>::Node* port)  { _input_ports.push_back(port); } ///< Preprocesser thread
	void add_output(Raul::List<PortImpl*>::Node* port) { _output_ports.push_back(port); } ///< Preprocessor thread
	Raul::List<PortImpl*>::Node* remove_port(const std::string& name);
	void                   clear_ports();

	void add_connection(SharedPtr<ConnectionImpl> c);

	SharedPtr<ConnectionImpl> remove_connection(const PortImpl* src_port, const PortImpl* dst_port);

	bool has_connection(const PortImpl* src_port, const PortImpl* dst_port) const;

	CompiledPatch* compiled_patch()                  { return _compiled_patch; }
	void           compiled_patch(CompiledPatch* cp) { _compiled_patch = cp; }

	Raul::Array<PortImpl*>* external_ports()                           { return _ports; }
	void                    external_ports(Raul::Array<PortImpl*>* pa) { _ports = pa; }

	CompiledPatch*          compile() const;
	Raul::Array<PortImpl*>* build_ports_array() const;

	/** Whether to run this patch's DSP bits in the audio thread */
	bool enabled() const { return _process; }
	void enable() { _process = true; }
	void disable();

	uint32_t internal_poly() const { return _internal_poly; }

private:
	inline void compile_recursive(NodeImpl* n, CompiledPatch* output) const;
	void process_parallel(ProcessContext& context);
	void process_single(ProcessContext& context);

	Engine&               _engine;
	uint32_t              _internal_poly;
	CompiledPatch*        _compiled_patch; ///< Accessed in audio thread only
	Connections           _connections;    ///< Accessed in preprocessing thread only
	Raul::List<PortImpl*> _input_ports;    ///< Accessed in preprocessing thread only
	Raul::List<PortImpl*> _output_ports;   ///< Accessed in preprocessing thread only
	Nodes                 _nodes;          ///< Accessed in preprocessing thread only
	bool                  _process;
};

/** Private helper for compile */
inline void
PatchImpl::compile_recursive(NodeImpl* n, CompiledPatch* output) const
{
	if (n == NULL || n->traversed())
		return;

	n->traversed(true);
	assert(output != NULL);

	for (Raul::List<NodeImpl*>::iterator i = n->providers()->begin(); i != n->providers()->end(); ++i)
		if ( ! (*i)->traversed() )
			compile_recursive((*i), output);

	output->push_back(CompiledNode(n, n->providers()->size(), n->dependants()));
}

} // namespace Ingen

#endif // INGEN_ENGINE_PATCHIMPL_HPP
