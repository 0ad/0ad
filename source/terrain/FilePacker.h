///////////////////////////////////////////////////////////////////////////////
//
// Name:		FilePacker.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _FILEPACKER_H
#define _FILEPACKER_H

#include <vector>
#include "res/res.h"
#include "CStr.h"

////////////////////////////////////////////////////////////////////////////////////////
// CFilePacker: class to assist in writing of binary files
class CFilePacker 
{
public:
	// CFilePacker exceptions
	class CError { };
	class CFileOpenError : public CError { };
	class CFileWriteError : public CError { };

public:
	// constructor
	CFilePacker();

	// Write: write out any packed data to file, using given version and magic bits
	void Write(const char* filename,u32 version,const char magicstr[4]);

	// PackRaw: pack given number of bytes onto the end of the data stream
	void PackRaw(const void* rawdata,u32 rawdatalen);
	// PackString: pack a string onto the end of the data stream
	void PackString(const CStr& str);

private:
	// the output data stream built during pack operations
	std::vector<u8> m_Data;
};

#endif