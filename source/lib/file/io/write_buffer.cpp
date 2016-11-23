/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "precompiled.h"
#include "lib/file/io/write_buffer.h"

#include "lib/bits.h"	// IsAligned
#include "lib/sysdep/cpu.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/file/io/io.h"


static const size_t BLOCK_SIZE = 512*KiB;


WriteBuffer::WriteBuffer()
	: m_capacity(pageSize), m_data((u8*)rtl_AllocateAligned(m_capacity, maxSectorSize), AlignedDeleter()), m_size(0)
{
}


void WriteBuffer::EnsureSufficientCapacity(size_t size)
{
	if(m_size + size > m_capacity)
	{
		m_capacity = round_up_to_pow2(m_size + size);
		shared_ptr<u8> newData;
		AllocateAligned(newData, m_capacity, maxSectorSize);
		memcpy(newData.get(), m_data.get(), m_size);
		m_data = newData;
	}
}


void WriteBuffer::Append(const void* data, size_t size)
{
	EnsureSufficientCapacity(size);
	memcpy(m_data.get() + m_size, data, size);
	m_size += size;
}


void WriteBuffer::Reserve(size_t size)
{
	EnsureSufficientCapacity(size);
	memset(m_data.get() + m_size, 0, size);
	m_size += size;
}


void WriteBuffer::Overwrite(const void* data, size_t size, size_t offset)
{
	ENSURE(offset+size < m_size);
	memcpy(m_data.get()+offset, data, size);
}


//-----------------------------------------------------------------------------
// UnalignedWriter
//-----------------------------------------------------------------------------

UnalignedWriter::UnalignedWriter(const PFile& file, off_t ofs)
	: m_file(file), m_alignedBuf((u8*)rtl_AllocateAligned(BLOCK_SIZE, maxSectorSize), AlignedDeleter())
{
	m_alignedOfs = round_down(ofs, (off_t)BLOCK_SIZE);
	const size_t misalignment = (size_t)(ofs - m_alignedOfs);
	if(misalignment)
	{
		io::Operation op(*m_file.get(), m_alignedBuf.get(), BLOCK_SIZE, m_alignedOfs);
		THROW_STATUS_IF_ERR(io::Run(op));
	}
	m_bytesUsed = misalignment;
}


UnalignedWriter::~UnalignedWriter()
{
	Flush();
}


Status UnalignedWriter::Append(const u8* data, size_t size) const
{
	while(size != 0)
	{
		// optimization: write directly from the input buffer, if possible
		const size_t alignedSize = (size / BLOCK_SIZE) * BLOCK_SIZE;
		if(m_bytesUsed == 0 && IsAligned(data, maxSectorSize) && alignedSize != 0)
		{
			io::Operation op(*m_file.get(), (void*)data, alignedSize, m_alignedOfs);
			RETURN_STATUS_IF_ERR(io::Run(op));
			m_alignedOfs += (off_t)alignedSize;
			data += alignedSize;
			size -= alignedSize;
		}

		const size_t chunkSize = std::min(size, BLOCK_SIZE-m_bytesUsed);
		memcpy(m_alignedBuf.get()+m_bytesUsed, data, chunkSize);
		m_bytesUsed += chunkSize;
		data += chunkSize;
		size -= chunkSize;

		if(m_bytesUsed == BLOCK_SIZE)
			RETURN_STATUS_IF_ERR(WriteBlock());
	}

	return INFO::OK;
}


void UnalignedWriter::Flush() const
{
	if(m_bytesUsed)
	{
		memset(m_alignedBuf.get()+m_bytesUsed, 0, BLOCK_SIZE-m_bytesUsed);
		(void)WriteBlock();
	}
}


Status UnalignedWriter::WriteBlock() const
{
	io::Operation op(*m_file.get(), m_alignedBuf.get(), BLOCK_SIZE, m_alignedOfs);
	RETURN_STATUS_IF_ERR(io::Run(op));
	m_alignedOfs += BLOCK_SIZE;
	m_bytesUsed = 0;
	return INFO::OK;
}
