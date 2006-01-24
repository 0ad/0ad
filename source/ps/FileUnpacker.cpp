///////////////////////////////////////////////////////////////////////////////
//
// Name:		FileUnpacker.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "precompiled.h"

#include "FileUnpacker.h"
#include "lib/res/res.h"
 
////////////////////////////////////////////////////////////////////////////////////////
// CFileUnpacker constructor
CFileUnpacker::CFileUnpacker()
{
	m_Buf = 0;
	m_Size = 0;
	m_UnpackPos = 0;
	m_Version = 0;
}

CFileUnpacker::~CFileUnpacker()
{
	file_buf_free(m_Buf);
}

////////////////////////////////////////////////////////////////////////////////////////
// Read: open and read in given file, check magic bits against those given; throw 
// variety of exceptions for missing files etc
void CFileUnpacker::Read(const char* filename,const char magicstr[4])
{
	// avoid vfs_load complaining about missing data files (which happens
	// too often). better to check here than squelch internal VFS error
	// reporting. we disable this in release mode to avoid a speed hit.
		// UPDATE: We don't disable this in release mode, because vfs_load now
		// complains about missing files when running in release
//#ifndef NDEBUG
	if(!vfs_exists(filename))
		throw CFileOpenError();
//#endif

	// load the whole thing into memory
	if(vfs_load(filename, m_Buf, m_Size) < 0)
		throw CFileOpenError();

	// make sure we read enough for the header
	if(m_Size < 12)
	{
		(void)file_buf_free(m_Buf);
		m_Buf = 0;
		m_Size = 0;
		throw CFileReadError();
	}

	// extract data from header
	u8* header = (u8*)m_Buf;
	char* magic = (char*)(header+0);
	// FIXME m_Version and datasize: Byte order? -- Simon
	m_Version = *(u32*)(header+4);
	u32 datasize = *(u32*)(header+8);

	// check we've got the right kind of file
	// .. and that we read exactly headersize+datasize
	if(strncmp(magic, magicstr, 4) != 0 ||
	   m_Size != 12+datasize)
	{
		(void)file_buf_free(m_Buf);
		m_Buf = 0;
		m_Size = 0;
		throw CFileTypeError();
	}

	m_UnpackPos = 12;
}

////////////////////////////////////////////////////////////////////////////////////////
// UnpackRaw: unpack given number of bytes from the input stream into the given array
//	- throws CFileEOFError if the end of the data stream is reached before the given 
// number of bytes have been read
void CFileUnpacker::UnpackRaw(void* rawdata,u32 rawdatalen)
{
	// got enough data to unpack?
	if (m_UnpackPos+rawdatalen<=m_Size)
	{
		// yes .. copy over
		void* src = (char*)m_Buf + m_UnpackPos;
		memcpy2(rawdata, src, rawdatalen);
		m_UnpackPos += rawdatalen;
	}
	else
		// nope - throw exception
		throw CFileEOFError();
}

////////////////////////////////////////////////////////////////////////////////////////
// UnpackString: unpack a string from the raw data stream
//	- throws CFileEOFError if eof is reached before the string length has been
// satisfied
void CFileUnpacker::UnpackString(CStr& result)
{
	// get string length
	u32 length;
	UnpackRaw(&length,sizeof(length)); // FIXME Byte order? -- Simon
	
	if (m_UnpackPos + length <= m_Size)
	{
		result=std::string((char *)m_Buf+m_UnpackPos, length);
		m_UnpackPos += length;
	}
	else
		throw CFileEOFError();
}

