///////////////////////////////////////////////////////////////////////////////
//
// Name:		FileUnpacker.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "FileUnpacker.h"
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////////////
// CFileUnpacker constructor
CFileUnpacker::CFileUnpacker() : m_UnpackPos(0), m_Version(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////
// Read: open and read in given file, check magic bits against those given; throw 
// variety of exceptions for missing files etc
void CFileUnpacker::Read(const char* filename,const char magicstr[4])
{
	FILE* fp=fopen(filename,"rb");
	if (!fp) {
		throw CFileOpenError();
	}

	// read magic bits
	char magic[4];
	if (fread(magic,sizeof(char)*4,1,fp)!=1) {
		fclose(fp);
		throw CFileReadError();
	}

	// check we've got the right kind of file
	if (strncmp(magic,magicstr,4)!=0) {
		// nope ..
		fclose(fp);
		throw CFileTypeError();
	}

	// get version
	if (fread(&m_Version,sizeof(m_Version),1,fp)!=1) {
		fclose(fp);
		throw CFileReadError();
	}

	// get size of anim data
	u32 datasize;
	if (fread(&datasize,sizeof(datasize),1,fp)!=1) {
		fclose(fp);
		throw CFileReadError();
	}

	// allocate memory and read in a big chunk of data
	m_Data.resize(datasize);
	if (fread(&m_Data[0],datasize,1,fp)!=1) {
		fclose(fp);
		throw CFileReadError();
	}

	// all done
	fclose(fp);
}

////////////////////////////////////////////////////////////////////////////////////////
// UnpackRaw: unpack given number of bytes from the input stream into the given array
//	- throws CFileEOFError if the end of the data stream is reached before the given 
// number of bytes have been read
void CFileUnpacker::UnpackRaw(void* rawdata,u32 rawdatalen)
{
	// got enough data to unpack?
	if (m_UnpackPos+rawdatalen<=m_Data.size()) {
		// yes .. copy over
		memcpy(rawdata,&m_Data[m_UnpackPos],rawdatalen);
		// advance pointer
		m_UnpackPos+=rawdatalen;
	} else {
		// nope - throw exception
		throw CFileEOFError();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// UnpackString: unpack a string from the raw data stream
void CFileUnpacker::UnpackString(CStr& result)
{
	// get string length
	u32 length;
	UnpackRaw(&length,sizeof(length));
	
	// read string into temporary buffer
	std::vector<char> tmp;
	tmp.resize(length+1);
	UnpackRaw(&tmp[0],length);
	tmp[length]='\0';
	
	// assign to output 
	result=&tmp[0];
}

