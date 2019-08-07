/* Copyright (C) 2019 Wildfire Games.
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

/*
 * endian-safe binary file IO helpers.
 */

#include "precompiled.h"

#include "FileIo.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "lib/byte_order.h"

#pragma pack(push, 1)

struct FileHeader
{
	char magic[4];
	u32 version_le;
	u32 payloadSize_le;	// = file size - sizeof(FileHeader)
};
cassert(sizeof(FileHeader) == 12);

#pragma pack(pop)


//-----------------------------------------------------------------------------
// CFilePacker

CFilePacker::CFilePacker(u32 version, const char magic[4])
{
	// put header in our data array.
	// (its payloadSize_le will be updated on every Pack*() call)
	FileHeader header;
	std::copy(magic, magic + 4, header.magic);
	write_le32(&header.version_le, version);
	write_le32(&header.payloadSize_le, 0);
	m_writeBuffer.Append(&header, sizeof(FileHeader));
}


CFilePacker::~CFilePacker()
{
}


void CFilePacker::Write(const VfsPath& filename)
{
	const size_t payloadSize = m_writeBuffer.Size() - sizeof(FileHeader);
	const u32 payloadSize_le = to_le32(u32_from_larger(payloadSize));
	m_writeBuffer.Overwrite(&payloadSize_le, sizeof(payloadSize_le), 0+offsetof(FileHeader, payloadSize_le));

	// write out all data (including header)
	const Status st = g_VFS->CreateFile(filename, m_writeBuffer.Data(), m_writeBuffer.Size());
	if (st < 0)
	{
		LOGERROR("Failed to write file '%s' with status '%lld'", filename.string8(), (long long)st);
		throw PSERROR_File_WriteFailed();
	}
}


void CFilePacker::PackRaw(const void* rawData, size_t rawSize)
{
	m_writeBuffer.Append(rawData, rawSize);
}

void CFilePacker::PackSize(size_t value)
{
	const u32 value_le32 = to_le32(u32_from_larger(value));
	PackRaw(&value_le32, sizeof(value_le32));
}

void CFilePacker::PackString(const CStr& str)
{
	const size_t length = str.length();
	PackSize(length);
	PackRaw(str.c_str(), length);
}


//-----------------------------------------------------------------------------
// CFileUnpacker

CFileUnpacker::CFileUnpacker()
	: m_bufSize(0), m_unpackPos(0), m_version(0)
{
}


CFileUnpacker::~CFileUnpacker()
{
}


void CFileUnpacker::Read(const VfsPath& filename, const char magic[4])
{
	// if the file doesn't exist, LoadFile would raise an annoying error dialog.
	// since this (unfortunately) happens so often, we perform a separate
	// check and "just" raise an exception for the caller to handle.
	// (this is nicer than somehow squelching internal VFS error reporting)
	if(!VfsFileExists(filename))
		throw PSERROR_File_OpenFailed();

	// load the whole thing into memory
	if(g_VFS->LoadFile(filename, m_buf, m_bufSize) < 0)
		throw PSERROR_File_OpenFailed();

	// make sure we read enough for the header
	if(m_bufSize < sizeof(FileHeader))
	{
		m_buf.reset();
		m_bufSize = 0;
		throw PSERROR_File_ReadFailed();
	}

	// extract data from header
	FileHeader* header = (FileHeader*)m_buf.get();
	m_version = read_le32(&header->version_le);
	const size_t payloadSize = (size_t)read_le32(&header->payloadSize_le);

	// check we've got the right kind of file
	// .. and that we read exactly headerSize+payloadSize
	if(strncmp(header->magic, magic, 4) != 0 || m_bufSize != sizeof(FileHeader)+payloadSize)
	{
		m_buf.reset();
		m_bufSize = 0;
		throw PSERROR_File_InvalidType();
	}

	m_unpackPos = sizeof(FileHeader);
}


void CFileUnpacker::UnpackRaw(void* rawData, size_t rawDataSize)
{
	// fail if reading past end of stream
	if (m_unpackPos+rawDataSize > m_bufSize)
		throw PSERROR_File_UnexpectedEOF();

	void* src = m_buf.get() + m_unpackPos;
	memcpy(rawData, src, rawDataSize);
	m_unpackPos += rawDataSize;
}


size_t CFileUnpacker::UnpackSize()
{
	u32 value_le32;
	UnpackRaw(&value_le32, sizeof(value_le32));
	return (size_t)to_le32(value_le32);
}


void CFileUnpacker::UnpackString(CStr8& result)
{
	const size_t length = UnpackSize();

	// fail if reading past end of stream
	if (m_unpackPos+length > m_bufSize)
		throw PSERROR_File_UnexpectedEOF();

	result = CStr((char*)m_buf.get()+m_unpackPos, length);
	m_unpackPos += length;
}
