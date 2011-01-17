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

#include "Stream.h"

namespace DatafileIO
{

	class SeekableInputStream_mem : public SeekableInputStream
	{
	public:
		// 'Release' provides common buffer-release mechanisms, called
		// when this stream is destroyed
		struct Releaser {
			virtual ~Releaser() {}
			virtual void release(char* buffer) = 0;
		};
		struct Releaser_None : public Releaser {
			virtual void release(char* /*buffer*/) {};
		};
		struct Releaser_StreamBuffer : public Releaser {
			Releaser_StreamBuffer(InputStream* s) : stream(s) {}
			InputStream* stream;
			virtual void release(char* buffer) {
				stream->ReleaseBuffer(buffer);
			}
		};
		struct Releaser_NewArray : public Releaser {
			virtual void release(char* buffer) {
				delete[] buffer;
			}
		};

		SeekableInputStream_mem(char* data, size_t size, Releaser* releaser);
		~SeekableInputStream_mem();
		virtual off_t Tell() const;
		virtual bool IsOk() const;
		virtual void Seek(off_t pos, Stream::whence mode);
		virtual size_t Read(void* buffer, size_t size);
		virtual bool AcquireBuffer(void*& buffer, size_t& size, size_t max_size);
		virtual void ReleaseBuffer(void* buffer);
	private:
		Releaser* m_Releaser;
		char* m_Data;
		size_t m_Size;
		off_t m_Cursor;
	};


	// Magically decompresses l33t-compressed files.
	class Maybel33tInputStream : public SeekableInputStream
	{
	public:
		// Take ownership of a SeekableInputStream
		Maybel33tInputStream(SeekableInputStream* stream);
		~Maybel33tInputStream();

		virtual off_t Tell() const;
		virtual bool IsOk() const;
		virtual void Seek(off_t pos, Stream::whence mode);
		virtual size_t Read(void* buffer, size_t size);
		virtual bool AcquireBuffer(void*& buffer, size_t& size, size_t max_size);
		virtual void ReleaseBuffer(void* buffer);
	private:
		SeekableInputStream* m_Stream;
	};
}
