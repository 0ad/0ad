///////////////////////////////////////////////////////////////////////////////
//
// Name:		FilePacker.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "precompiled.h"

#include "FilePacker.h"

#include <string.h>
#include "lib/res/file/vfs.h"

 
////////////////////////////////////////////////////////////////////////////////////////
// CFilePacker constructor
// rationale for passing in version + signature here: see header
CFilePacker::CFilePacker(u32 version, const char magicstr[4]) 
{
	// put header in our data array.
	// (size will be updated on every Pack*() call)
	m_Data.resize(12);
	u8* header = (u8*)&m_Data[0];
	strncpy((char*)(header+0), magicstr, 4);	// not 0-terminated => no _s
	*(u32*)(header+4) = version;
	*(u32*)(header+8) = 0;	// datasize
	// FIXME m_Version: Byte order? -- Simon
}

////////////////////////////////////////////////////////////////////////////////////////
// Write: write out to file all packed data added so far
void CFilePacker::Write(const char* filename)
{
#ifndef SCED
	// write out all data (including header)
	if(vfs_store(filename, &m_Data[0], m_Data.size(), FILE_NO_AIO) < 0)
		throw CFileWriteError();
#else

	// 'filename' is already an absolute path, so don't use file_make_full_native_path
	FILE* fp=fopen(filename,"wb");
	if (!fp) {
		throw CFileOpenError();
	}

	// write out one big chunk of data (includes header)
	if (fwrite(&m_Data[0],m_Data.size(),1,fp)!=1) {
		fclose(fp);
		throw CFileWriteError();
	}

	// all done
	fclose(fp);
#endif
}


////////////////////////////////////////////////////////////////////////////////////////
// PackRaw: pack given number of bytes onto the end of the data stream
void CFilePacker::PackRaw(const void* rawdata,u32 rawdatalen)
{
	u32 start=(u32)m_Data.size();
	m_Data.resize(m_Data.size()+rawdatalen);
	memcpy2(&m_Data[start],rawdata,rawdatalen);
	
	*(u32*)&m_Data[8] += rawdatalen;	// FIXME byte order?
}

////////////////////////////////////////////////////////////////////////////////////////
// PackString: pack a string onto the end of the data stream
void CFilePacker::PackString(const CStr& str)
{
	u32 len=(u32)str.Length();
	PackRaw(&len,sizeof(len));
	PackRaw((const char*) str,len);
}

