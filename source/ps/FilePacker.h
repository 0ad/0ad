/**
 * =========================================================================
 * File        : FilePacker.h
 * Project     : 0 A.D.
 * Description : Resizable buffer, for writing binary files
 * =========================================================================
 */

#ifndef INCLUDED_FILEPACKER
#define INCLUDED_FILEPACKER

#include <vector>
#include "CStr.h"
#include "lib/file/vfs/vfs_path.h"

#include "ps/Errors.h"
#include "ps/Filesystem.h"	// WriteBuffer

#ifndef ERROR_GROUP_FILE_DEFINED
#define ERROR_GROUP_FILE_DEFINED
// FileUnpacker.h defines these too
ERROR_GROUP(File);
ERROR_TYPE(File, OpenFailed);
#endif
ERROR_TYPE(File, WriteFailed);

////////////////////////////////////////////////////////////////////////////////////////
// CFilePacker: class to assist in writing of binary files.
// basically a resizeable buffer that allows adding raw data and strings;
// upon calling Write(), everything is written out to disk.
class CFilePacker 
{
public:
	// constructor
	// adds version and signature (i.e. the header) to the buffer.
	// this means Write() can write the entire buffer to file in one go,
	// which is simpler and more efficient than writing in pieces.
	CFilePacker(u32 version, const char magicstr[4]);

	// Write: write out to file all packed data added so far
	void Write(const VfsPath& filename);

	// PackRaw: pack given number of bytes onto the end of the data stream
	void PackRaw(const void* rawdata, size_t rawdatalen);
	// PackString: pack a string onto the end of the data stream
	void PackString(const CStr& str);

private:
	// the output data stream built during pack operations.
	// contains the header, so we can write this out in one go.
	WriteBuffer m_writeBuffer;
};

#endif
