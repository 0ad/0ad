///////////////////////////////////////////////////////////////////////////////
//
// Name:		FilePacker.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "precompiled.h"

#include "FilePacker.h"
#include <stdio.h>

 
////////////////////////////////////////////////////////////////////////////////////////
// CFilePacker constructor
CFilePacker::CFilePacker() 
{
}

////////////////////////////////////////////////////////////////////////////////////////
// Write: write out any packed data to file, using given version and magic bits
void CFilePacker::Write(const char* filename,u32 version,const char magicstr[4])
{
	FILE* fp=fopen(filename,"wb");
	if (!fp) {
		throw CFileOpenError();
	}

	// write magic bits
	if (fwrite(magicstr,sizeof(char)*4,1,fp)!=1) {
		fclose(fp);
		throw CFileWriteError();
	}

	// write version
	if (fwrite(&version,sizeof(version),1,fp)!=1) {
		fclose(fp);
		throw CFileWriteError();
	}

	// get size of data
	u32 datasize=m_Data.size();
	if (fwrite(&datasize,sizeof(datasize),1,fp)!=1) {
		fclose(fp);
		throw CFileWriteError();
	}

	// write out one big chunk of data
	if (fwrite(&m_Data[0],datasize,1,fp)!=1) {
		fclose(fp);
		throw CFileWriteError();
	}

	// all done
	fclose(fp);
}


////////////////////////////////////////////////////////////////////////////////////////
// PackRaw: pack given number of bytes onto the end of the data stream
void CFilePacker::PackRaw(const void* rawdata,u32 rawdatalen)
{
	u32 start=m_Data.size();
	m_Data.resize(m_Data.size()+rawdatalen);
	memcpy(&m_Data[start],rawdata,rawdatalen);
	
}

////////////////////////////////////////////////////////////////////////////////////////
// PackString: pack a string onto the end of the data stream
void CFilePacker::PackString(const CStr& str)
{
	u32 len=str.Length();
	PackRaw(&len,sizeof(len));
	PackRaw((const char*) str,len);
}

