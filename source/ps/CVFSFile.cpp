#include "precompiled.h"

#include "CVFSFile.h"

#include "lib/res/mem.h"
#include "lib/res/vfs.h"
#include "ps/CLogger.h"

#define LOG_CATEGORY "file"

CVFSFile::CVFSFile() : m_Handle(0) {}

CVFSFile::~CVFSFile()
{
	if (m_Handle)
		mem_free_h(m_Handle);
}

PSRETURN CVFSFile::Load(const char* filename, uint flags /* = 0 */)
{
	if (m_Handle)
	{
		// Load should never be called more than once, so complain
		debug_warn("Mustn't open files twice");
		return PSRETURN_CVFSFile_AlreadyLoaded;
	}

	m_Handle = vfs_load(filename, m_Buffer, m_BufferSize, flags);
	if (m_Handle <= 0)
	{
		LOG(ERROR, LOG_CATEGORY, "CVFSFile: file %s couldn't be opened (vfs_load: %lld)", filename, m_Handle);
		return PSRETURN_CVFSFile_LoadFailed;
	}

	return PSRETURN_OK;
}

const void* CVFSFile::GetBuffer() const
{
	// Die in a very obvious way, to avoid subtle problems caused by
	// accidentally forgetting to check that the open succeeded
	if (!m_Handle)
	{
		debug_warn("GetBuffer() called with no file loaded");
		throw PSERROR_CVFSFile_InvalidBufferAccess();
	}

	return m_Buffer;
}

const size_t CVFSFile::GetBufferSize() const
{
	return m_BufferSize;
}

CStr CVFSFile::GetAsString() const
{
	return std::string((char*)GetBuffer(), GetBufferSize());
}
