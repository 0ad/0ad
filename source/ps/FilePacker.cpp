/**
 * =========================================================================
 * File        : FilePacker.cpp
 * Project     : 0 A.D.
 * Description : Resizable buffer, for writing binary files
 * =========================================================================
 */

#include "precompiled.h"

#include "FilePacker.h"
#include "ps/Filesystem.h"
#include "lib/byte_order.h"

#include <string.h>


////////////////////////////////////////////////////////////////////////////////////////
// CFilePacker constructor
// rationale for passing in version + signature here: see header
CFilePacker::CFilePacker(u32 version, const char magicstr[4])
{
	// put header in our data array.
	// (size will be updated on every Pack*() call)
	char header[12];
	strncpy(header+0, magicstr, 4);	// not 0-terminated => no _s
	write_le32(header+4, version);
	write_le32(header+8, 0);	// datasize
	m_writeBuffer.Append(header, 12);
}

////////////////////////////////////////////////////////////////////////////////////////
// Write: write out to file all packed data added so far
void CFilePacker::Write(const VfsPath& filename)
{
	const u32 size_le = to_le32(u32_from_larger(m_writeBuffer.Size() - 12));
	m_writeBuffer.Overwrite(&size_le, sizeof(size_le), 8);

	// write out all data (including header)
	if(g_VFS->CreateFile(filename, m_writeBuffer.Data(), m_writeBuffer.Size()) < 0)
		throw PSERROR_File_WriteFailed();
}


////////////////////////////////////////////////////////////////////////////////////////
// PackRaw: pack given number of bytes onto the end of the data stream
void CFilePacker::PackRaw(const void* rawData, size_t rawSize)
{
	m_writeBuffer.Append(rawData, rawSize);
}

////////////////////////////////////////////////////////////////////////////////////////
// PackString: pack a string onto the end of the data stream
void CFilePacker::PackString(const CStr& str)
{
	const size_t length = str.length();
	const u32 length_le = to_le32(u32_from_larger(length));
	PackRaw(&length_le, sizeof(length_le));
	PackRaw((const char*)str, length);
}
