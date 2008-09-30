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

#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include <cstddef>
#include <cassert>
#include <boost/utility.hpp>
#include "types.hpp"
#include "Buffer.hpp"

namespace Ingen {

	
class AudioBuffer : public Buffer
{
public:
	AudioBuffer(size_t capacity);

	void clear();
	void set_value(Sample val, FrameTime cycle_start, FrameTime time);
	void set_block(Sample val, size_t start_offset, size_t end_offset);
	void scale(Sample val, size_t start_sample, size_t end_sample);
	void copy(const Buffer* src, size_t start_sample, size_t end_sample);
	void accumulate(const AudioBuffer* src, size_t start_sample, size_t end_sample);
	
	bool join(Buffer* buf);
	void unjoin();
	
	/** For driver use only!! */
	void set_data(Sample* data);
	
	inline const void* raw_data() const { return _data; }
	inline void*       raw_data()       { return _data; }
	
	inline Sample* data() const { return _data; }
	
	inline Sample& value_at(size_t offset) const
		{ assert(offset < _size); return data()[offset]; }
	
	void prepare_read(FrameTime start, SampleCount nframes);
	void prepare_write(FrameTime start, SampleCount nframes) {}
	
	void rewind() const {}
	void resize(size_t size);
	
	void      filled_size(size_t size) { _filled_size = size; }
	size_t    filled_size() const { return _filled_size; }
	size_t    size()        const { return _size; }

private:
	enum State { OK, HALF_SET_CYCLE_1, HALF_SET_CYCLE_2 };

	void allocate();
	void deallocate();

	Sample*      _data;        ///< Used data pointer (probably same as _local_data)
	Sample*      _local_data;  ///< Locally allocated buffer (possibly unused if joined or set_data used)
	size_t       _size;        ///< Allocated buffer size
	size_t       _filled_size; ///< Usable buffer size (for MIDI ports etc)
	State        _state;       ///< State of buffer for setting values next cycle
	Sample       _set_value;   ///< Value set by @ref set (may need to be set next cycle)
	FrameTime    _set_time;    ///< Time _set_value was set (to reset next cycle)
};


} // namespace Ingen

#endif // AUDIOBUFFER_H