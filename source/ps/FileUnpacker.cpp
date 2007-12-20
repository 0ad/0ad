/**
 * =========================================================================
 * File        : FileUnpacker.cpp
 * Project     : 0 A.D.
 * Description : Buffer and 'stream' for reading binary files
 * =========================================================================
 */

#include "precompiled.h"

#include "ps/FileUnpacker.h"
#include "ps/CStr.h"
#include "ps/Filesystem.h"
#include "lib/byte_order.h"
 
////////////////////////////////////////////////////////////////////////////////////////
// CFileUnpacker constructor
CFileUnpacker::CFileUnpacker()
{
	m_Size = 0;
	m_UnpackPos = 0;
	m_Version = 0;
}

CFileUnpacker::~CFileUnpacker()
{
}

////////////////////////////////////////////////////////////////////////////////////////
// Read: open and read in given file, check magic bits against those given; throw 
// variety of exceptions for missing files etc
void CFileUnpacker::Read(const char* filename, const char magicstr[4])
{
	// avoid vfs_load complaining about missing data files (which happens
	// too often). better to check here than squelch internal VFS error
	// reporting. we disable this in release mode to avoid a speed hit.
	// UPDATE: We don't disable this in release mode, because vfs_load now
	// complains about missing files when running in release
//#ifndef NDEBUG
	if(!FileExists(filename))
		throw PSERROR_File_OpenFailed();
//#endif

	// load the whole thing into memory
	if(g_VFS->LoadFile(filename, m_Buf, m_Size) < 0)
		throw PSERROR_File_OpenFailed();

	// make sure we read enough for the header
	if(m_Size < 12)
	{
		m_Buf.reset();
		m_Size = 0;
		throw PSERROR_File_ReadFailed();
	}

	// extract data from header
	u8* header = (u8*)m_Buf.get();
	char* magic = (char*)(header+0);
	// FIXME m_Version and datasize: Byte order? -- Simon
	m_Version = read_le32(header+4);
	u32 datasize = read_le32(header+8);

	// check we've got the right kind of file
	// .. and that we read exactly headersize+datasize
	if(strncmp(magic, magicstr, 4) != 0 || m_Size != 12+datasize)
	{
		m_Buf.reset();
		m_Size = 0;
		throw PSERROR_File_InvalidType();
	}

	m_UnpackPos = 12;
}

////////////////////////////////////////////////////////////////////////////////////////
// UnpackRaw: unpack given number of bytes from the input stream into the given array
//	- throws CFileEOFError if the end of the data stream is reached before the given 
// number of bytes have been read
void CFileUnpacker::UnpackRaw(void* rawdata, size_t rawdatalen)
{
	// fail if reading past end of stream
	if (m_UnpackPos+rawdatalen > m_Size)
		throw PSERROR_File_UnexpectedEOF();

	void* src = m_Buf.get() + m_UnpackPos;
	cpu_memcpy(rawdata, src, rawdatalen);
	m_UnpackPos += rawdatalen;
}

////////////////////////////////////////////////////////////////////////////////////////
// UnpackString: unpack a string from the raw data stream
//	- throws CFileEOFError if eof is reached before the string length has been
// satisfied
void CFileUnpacker::UnpackString(CStr& result)
{
	// get string length
	u32 length_le;
	UnpackRaw(&length_le, sizeof(length_le));
	const size_t length = to_le32(length_le);

	// fail if reading past end of stream
	if (m_UnpackPos + length > m_Size)
		throw PSERROR_File_UnexpectedEOF();

	result = CStr((char*)m_Buf.get()+m_UnpackPos, length);
	m_UnpackPos += length;
}
