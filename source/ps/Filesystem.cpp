#include "precompiled.h"
#include "Filesystem.h"

#include "ps/CLogger.h"

#define LOG_CATEGORY "file"


PIVFS g_VFS;

bool FileExists(const char* pathname)
{
	return g_VFS->GetFileInfo(pathname, 0) == INFO::OK;
}

bool FileExists(const VfsPath& pathname)
{
	return g_VFS->GetFileInfo(pathname, 0) == INFO::OK;
}



CVFSFile::CVFSFile()
{
}

CVFSFile::~CVFSFile()
{
}

PSRETURN CVFSFile::Load(const VfsPath& filename)
{
	// Load should never be called more than once, so complain
	if (m_Buffer)
	{
		debug_assert(0);
		return PSRETURN_CVFSFile_AlreadyLoaded;
	}

	LibError ret = g_VFS->LoadFile(filename, m_Buffer, m_BufferSize);
	if (ret != INFO::OK)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "CVFSFile: file %s couldn't be opened (vfs_load: %d)", filename.string().c_str(), ret);
		return PSRETURN_CVFSFile_LoadFailed;
	}

	return PSRETURN_OK;
}

const u8* CVFSFile::GetBuffer() const
{
	// Die in a very obvious way, to avoid subtle problems caused by
	// accidentally forgetting to check that the open succeeded
	if (!m_Buffer)
	{
		debug_warn("GetBuffer() called with no file loaded");
		throw PSERROR_CVFSFile_InvalidBufferAccess();
	}

	return m_Buffer.get();
}

size_t CVFSFile::GetBufferSize() const
{
	return m_BufferSize;
}

CStr CVFSFile::GetAsString() const
{
	return std::string((char*)GetBuffer(), GetBufferSize());
}
