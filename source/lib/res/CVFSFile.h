// OO wrapper around VFS file Handles, to simplify common usages

#include "lib/res/h_mgr.h"
#include "ps/CStr.h"

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
	PSRETURN Load(const char* filename);

	// These die if called when no file has been successfully loaded.
	const void* GetBuffer() const;
	const size_t GetBufferSize() const;
	CStr GetAsString() const;

private:
	Handle m_Handle;
	void* m_Buffer;
	size_t m_BufferSize;
};