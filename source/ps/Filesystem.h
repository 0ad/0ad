#ifndef INCLUDED_FILESYSTEM
#define INCLUDED_FILESYSTEM

#include "lib/path_util.h"
#include "lib/file/file.h"
#include "lib/file/io/io.h"
#include "lib/file/vfs/vfs.h"
#include "lib/file/file_system_util.h"
#include "lib/file/io/write_buffer.h"

#include "ps/CStr.h"
#include "ps/Errors.h"

extern PIVFS g_VFS;

extern bool FileExists(const char* pathname);
extern bool FileExists(const VfsPath& pathname);

ERROR_GROUP(CVFSFile);
ERROR_TYPE(CVFSFile, LoadFailed);
ERROR_TYPE(CVFSFile, AlreadyLoaded);
ERROR_TYPE(CVFSFile, InvalidBufferAccess);

// Reads a file, then gives read-only access to the contents
class CVFSFile
{
public:
	CVFSFile();
	~CVFSFile();

	// Returns either PSRETURN_OK or PSRETURN_CVFSFile_LoadFailed.
	// Dies if a file has already been successfully loaded.
	PSRETURN Load(const VfsPath& filename);

	// These die if called when no file has been successfully loaded.
	const u8* GetBuffer() const;
	size_t GetBufferSize() const;
	CStr GetAsString() const;

private:
	shared_ptr<u8> m_Buffer;
	size_t m_BufferSize;
};

#endif	// #ifndef INCLUDED_FILESYSTEM
