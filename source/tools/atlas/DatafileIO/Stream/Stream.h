/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_STREAM
#define INCLUDED_STREAM

/*
	Stream: A system for input/output of data, particularly with chained streams
	(e.g. file -> zlib decompressor -> data processor).
	
	Similar to wxWidget's streams, but rewritten so that code can use this without
	requiring wx.

	TODO: This concept might not actually be any good; consider alternatives.
*/

#include <sys/types.h>
#include <limits>

namespace DatafileIO
{
	class Stream
	{
	public:
		enum whence { FROM_START, FROM_END, FROM_CURRENT };

		virtual ~Stream() {}
		virtual off_t Tell() const = 0;
		virtual bool IsOk() const = 0;
	};

	class SeekableStream
	{
	public:
		virtual ~SeekableStream() {}
		virtual void Seek(off_t pos, Stream::whence mode) = 0;
	};


	class InputStream : public Stream
	{
	public:
		// Try to read up to 'size' bytes into buffer, and return the amount
		// actually read.
		virtual size_t Read(void* buffer, size_t size) = 0;

		// Sets 'buffer' and 'size' to point to the data from the current cursor
		// position up to max_size bytes (or the end of the file, whichever is
		// reached first) and returns true if successful.
		// If a derived stream doesn't implement these, a default (which just
		// calls Read to get all the data) will be used instead. (It is implemented
		// in e.g. streams that already have a pointer to all the data, so they
		// don't need to do any memory copying.)
		virtual bool AcquireBuffer(void*& buffer, size_t& size, size_t max_size = std::numeric_limits<size_t>::max());
		virtual void ReleaseBuffer(void* buffer);
	};

	class OutputStream : public Stream
	{
	public:
		virtual void Write(const void* buffer, size_t size) = 0;
	};


	// Specialisations to indicate that a stream allows seeking.
	class SeekableInputStream : public InputStream, public SeekableStream
	{
	};

	class SeekableOutputStream : public OutputStream, public SeekableStream
	{
	};

}

#endif // INCLUDED_STREAM
