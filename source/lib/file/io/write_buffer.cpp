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

#include "precompiled.h"
#include "write_buffer.h"

#include "lib/bits.h"	// IsAligned
#include "lib/sysdep/cpu.h"
#include "io.h"
#include "io_align.h"


WriteBuffer::WriteBuffer()
	: m_capacity(4096), m_data(io_Allocate(m_capacity)), m_size(0)
{
}


void WriteBuffer::Append(const void* data, size_t size)
{
	if(m_size + size > m_capacity)
	{
		m_capacity = round_up_to_pow2((size_t)(m_size + size));
		shared_ptr<u8> newData = io_Allocate(m_capacity);
		cpu_memcpy(newData.get(), m_data.get(), m_size);
		m_data = newData;
	}

	cpu_memcpy(m_data.get() + m_size, data, size);
	m_size += size;
}


void WriteBuffer::Overwrite(const void* data, size_t size, size_t offset)
{
	debug_assert(offset+size < m_size);
	cpu_memcpy(m_data.get()+offset, data, size);
}


//-----------------------------------------------------------------------------
// UnalignedWriter
//-----------------------------------------------------------------------------

UnalignedWriter::UnalignedWriter(const PFile& file, off_t ofs)
	: m_file(file), m_alignedBuf(io_Allocate(BLOCK_SIZE))
{
	m_alignedOfs = AlignedOffset(ofs);
	const size_t misalignment = (size_t)(ofs - m_alignedOfs);
	if(misalignment)
		io_ReadAligned(m_file, m_alignedOfs, m_alignedBuf.get(), BLOCK_SIZE);
	m_bytesUsed = misalignment;
}


UnalignedWriter::~UnalignedWriter()
{
	Flush();
}


LibError UnalignedWriter::Append(const u8* data, size_t size) const
{
	while(size != 0)
	{
		// optimization: write directly from the input buffer, if possible
		const size_t alignedSize = (size / BLOCK_SIZE) * BLOCK_SIZE;
		if(m_bytesUsed == 0 && IsAligned(data, SECTOR_SIZE) && alignedSize != 0)
		{
			RETURN_ERR(io_WriteAligned(m_file, m_alignedOfs, data, alignedSize));
			m_alignedOfs += (off_t)alignedSize;
			data += alignedSize;
			size -= alignedSize;
		}

		const size_t chunkSize = std::min(size, BLOCK_SIZE-m_bytesUsed);
		cpu_memcpy(m_alignedBuf.get()+m_bytesUsed, data, chunkSize);
		m_bytesUsed += chunkSize;
		data += chunkSize;
		size -= chunkSize;

		if(m_bytesUsed == BLOCK_SIZE)
			RETURN_ERR(WriteBlock());
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


LibError UnalignedWriter::WriteBlock() const
{
	RETURN_ERR(io_WriteAligned(m_file, m_alignedOfs, m_alignedBuf.get(), BLOCK_SIZE));
	m_alignedOfs += BLOCK_SIZE;
	m_bytesUsed = 0;
	return INFO::OK;
}
