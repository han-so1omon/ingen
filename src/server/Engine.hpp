/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_ENGINE_HPP
#define INGEN_ENGINE_ENGINE_HPP

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <random>

#include "ingen/Clock.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/Properties.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"

#include "Event.hpp"
#include "Load.hpp"

namespace Raul {
class Maid;
class RingBuffer;
}

namespace ingen {

class AtomReader;
class Interface;
class Log;
class Store;
class World;

namespace server {

class BlockFactory;
class Broadcaster;
class BufferFactory;
class ControlBindings;
class Driver;
class EventWriter;
class GraphImpl;
class LV2Options;
class PostProcessor;
class PreProcessor;
class RunContext;
class SocketListener;
class Task;
class UndoStack;
class Worker;

/**
   The engine which executes the process graph.

   This is a simple class that provides pointers to the various components
   that make up the engine implementation.  In processes with a local engine,
   it can be accessed via the ingen::World.

   @ingroup engine
*/
class INGEN_API Engine : public EngineBase
{
public:
	explicit Engine(ingen::World* world);
	virtual ~Engine();

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	// EngineBase methods
	void init(double sample_rate, uint32_t block_length, size_t seq_size) override;
	bool supports_dynamic_ports() const override;
	bool activate() override;
	void deactivate() override;
	bool pending_events() const override;
	unsigned run(uint32_t sample_count) override;
	void quit() override;
	bool main_iteration() override;
	void register_client(SPtr<Interface> client) override;
	bool unregister_client(SPtr<Interface> client) override;

	void listen() override;

	/** Return a random [0..1] float with uniform distribution */
	float frand() { return _uniform_dist(_rand_engine); }

	void set_driver(SPtr<Driver> driver);

	/** Return the frame time to execute an event that arrived now.
	 *
	 * This aims to return a time one cycle from "now", so that events ideally
	 * have 1 cycle of latency with no jitter.
	 */
	SampleCount event_time();

	/** Return the time this cycle began processing in microseconds.
	 *
	 * This value is comparable to the value returned by current_time().
	 */
	inline uint64_t cycle_start_time(const RunContext& context) const {
		return _cycle_start_time;
	}

	/** Return the current time in microseconds. */
	uint64_t current_time() const;

	/** Reset the load statistics (when the expected DSP load changes). */
	void reset_load();

	/** Enqueue an event to be processed (non-realtime threads only). */
	void enqueue_event(Event* ev, Event::Mode mode=Event::Mode::NORMAL);

	/** Process events (process thread only). */
	unsigned process_events();

	/** Process all events (no RT limits). */
	unsigned process_all_events();

	ingen::World* world() const { return _world; }
	Log&          log()   const;

	const SPtr<Interface>&       interface()        const { return _interface; }
	const SPtr<EventWriter>&     event_writer()     const { return _event_writer; }
	const UPtr<AtomReader>&      atom_interface()   const { return _atom_interface; }
    const UPtr<BlockFactory>&    block_factory()    const { return _block_factory; }
    const UPtr<Broadcaster>&     broadcaster()      const { return _broadcaster; }
    const UPtr<BufferFactory>&   buffer_factory()   const { return _buffer_factory; }
    const UPtr<ControlBindings>& control_bindings() const { return _control_bindings; }
    const SPtr<Driver>&          driver()           const { return _driver; }
    const UPtr<PostProcessor>&   post_processor()   const { return _post_processor; }
    const UPtr<Raul::Maid>&      maid()             const { return _maid; }
    const UPtr<UndoStack>&       undo_stack()       const { return _undo_stack; }
    const UPtr<UndoStack>&       redo_stack()       const { return _redo_stack; }
    const UPtr<Worker>&          worker()           const { return _worker; }
    const UPtr<Worker>&          sync_worker()      const { return _sync_worker; }

    GraphImpl* root_graph() const { return _root_graph; }
	void       set_root_graph(GraphImpl* graph);

	RunContext& run_context() { return *_run_contexts[0]; }

	void flush_events(const std::chrono::milliseconds& sleep_ms) override;
	void advance(SampleCount nframes) override;
	void locate(FrameTime s, SampleCount nframes) override;

	void  emit_notifications(FrameTime end);
	bool  pending_notifications();
	bool  wait_for_tasks();
	void  signal_tasks_available();
	Task* steal_task(unsigned start_thread);

	SPtr<Store> store() const;

	SampleRate  sample_rate() const;
	SampleCount block_length() const;
	size_t      sequence_size() const;
	size_t      event_queue_size() const;

	size_t n_threads()      const { return _run_contexts.size(); }
	bool   atomic_bundles() const { return _atomic_bundles; }
	bool   activated()      const { return _activated; }

	Properties load_properties() const;

private:
	ingen::World* _world;

	SPtr<LV2Options>      _options;
	UPtr<BufferFactory>   _buffer_factory;
	UPtr<Raul::Maid>      _maid;
	SPtr<Driver>          _driver;
	UPtr<Worker>          _worker;
	UPtr<Worker>          _sync_worker;
	UPtr<Broadcaster>     _broadcaster;
	UPtr<ControlBindings> _control_bindings;
	UPtr<BlockFactory>    _block_factory;
	UPtr<UndoStack>       _undo_stack;
	UPtr<UndoStack>       _redo_stack;
	UPtr<PostProcessor>   _post_processor;
	UPtr<PreProcessor>    _pre_processor;
	UPtr<SocketListener>  _listener;
	SPtr<EventWriter>     _event_writer;
	SPtr<Interface>       _interface;
	UPtr<AtomReader>      _atom_interface;
	GraphImpl*            _root_graph;

	std::vector<Raul::RingBuffer*> _notifications;
	std::vector<RunContext*>       _run_contexts;
	uint64_t                       _cycle_start_time;
	Load                           _run_load;
	Clock                          _clock;

	std::mt19937                          _rand_engine;
	std::uniform_real_distribution<float> _uniform_dist;

	std::condition_variable _tasks_available;
	std::mutex              _tasks_mutex;

	bool _quit_flag;
	bool _reset_load_flag;
	bool _atomic_bundles;
	bool _activated;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_ENGINE_HPP
