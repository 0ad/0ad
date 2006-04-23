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
#include "lib/types.h"
#include "lib/res/file/file.h"
#include "CStr.h"

#include "ps/Errors.h"
#ifndef ERROR_GROUP_FILE_DEFINED
#define ERROR_GROUP_FILE_DEFINED
// FilePacker.h defines these too
ERROR_GROUP(File);
ERROR_TYPE(File, OpenFailed);
#endif
ERROR_TYPE(File, InvalidType);
ERROR_TYPE(File, InvalidVersion);
ERROR_TYPE(File, ReadFailed);
ERROR_TYPE(File, UnexpectedEOF);

////////////////////////////////////////////////////////////////////////////////
// CFileUnpacker: class to assist in reading of binary files
class CFileUnpacker
{
public:
	// constructor
	CFileUnpacker();

	~CFileUnpacker();
	
	// Read: open and read in given file, check magic bits against those given; throw 
	// variety of exceptions for missing files etc
	void Read(const char* filename, const char magicstr[4]);
	
	// GetVersion: return stored file version
	u32 GetVersion() const { return m_Version; }

	// UnpackRaw: unpack given number of bytes from the input stream into the given array
	//	- throws PSERROR_File_UnexpectedEOF if the end of the data stream is reached before
	// the given number of bytes have been read
	void UnpackRaw(void* rawdata, u32 rawdatalen);
	// UnpackString: unpack a string from the raw data stream
	void UnpackString(CStr& result);

private:
	// the data read from file and used during unpack operations
	FileIOBuf m_Buf;
	size_t m_Size;
	// current unpack position in stream
	u32 m_UnpackPos;
	// version of the file currently being read
	u32 m_Version;
};

#endif
