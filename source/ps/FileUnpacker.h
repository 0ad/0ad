///////////////////////////////////////////////////////////////////////////////
//
// Name:		FileUnpacker.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _FILEUNPACKER_H
#define _FILEUNPACKER_H

#include <vector>
#include "res/res.h"
#include "CStr.h"

////////////////////////////////////////////////////////////////////////////////
// CFileUnpacker: class to assist in reading of binary files
class CFileUnpacker 
{
public:
	// exceptions thrown by class
	class CError { };
	class CFileTypeError : public CError { };
	class CFileVersionError : public CError { };
	class CFileOpenError : public CError { };
	class CFileReadError : public CError { };
	class CFileEOFError : public CError { };

public:
	// constructor
	CFileUnpacker();
	
	// Read: open and read in given file, check magic bits against those given; throw 
	// variety of exceptions for missing files etc
	void Read(const char* filename,const char magicstr[4]);
	
	// GetVersion: return stored file version
	u32 GetVersion() const { return m_Version; }

	// UnpackRaw: unpack given number of bytes from the input stream into the given array
	//	- throws CFileEOFError if the end of the data stream is reached before the given 
	// number of bytes have been read
	void UnpackRaw(void* rawdata,u32 rawdatalen);
	// UnpackString: unpack a string from the raw data stream
	void UnpackString(CStr& result);

private:
	// the input data stream read from file and used during unpack operations
	std::vector<u8> m_Data;
	// current unpack position in stream
	u32 m_UnpackPos;
	// version of the file currently being read
	u32 m_Version;
};

#endif