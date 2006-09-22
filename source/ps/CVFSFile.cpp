#include "precompiled.h"

#include "CVFSFile.h"

#include "lib/res/mem.h"
#include "lib/res/file/vfs.h"
#include "ps/CLogger.h"

#define LOG_CATEGORY "file"

// (for only-call-once check)
CVFSFile::CVFSFile() : m_Buffer(0) {}

CVFSFile::~CVFSFile()
{
	(void)file_buf_free(m_Buffer);
}

PSRETURN CVFSFile::Load(const char* filename, uint flags /* = 0 */)
{
	if (m_Buffer)
	{
		// Load should never be called more than once, so complain
		debug_warn("Mustn't open files twice");
		return PSRETURN_CVFSFile_AlreadyLoaded;
	}

	LibError ret = vfs_load(filename, m_Buffer, m_BufferSize, flags);
	if (ret != INFO::OK)
	{
		LOG(ERROR, LOG_CATEGORY, "CVFSFile: file %s couldn't be opened (vfs_load: %d)", filename, ret);
		return PSRETURN_CVFSFile_LoadFailed;
	}

	return PSRETURN_OK;
}

const void* CVFSFile::GetBuffer() const
{
	// Die in a very obvious way, to avoid subtle problems caused by
	// accidentally forgetting to check that the open succeeded
	if (!m_Buffer)
	{
		debug_warn("GetBuffer() called with no file loaded");
		throw PSERROR_CVFSFile_InvalidBufferAccess();
	}

	return m_Buffer;
}

size_t CVFSFile::GetBufferSize() const
{
	return m_BufferSize;
}

CStr CVFSFile::GetAsString() const
{
	return std::string((char*)GetBuffer(), GetBufferSize());
}
