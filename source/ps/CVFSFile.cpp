#include "precompiled.h"

#include "CVFSFile.h"

#include "lib/res/mem.h"
#include "lib/res/vfs.h"
#include "ps/CLogger.h"

#define LOG_CATEGORY "file"

CVFSFile::CVFSFile() : m_Handle(0) {}

CVFSFile::~CVFSFile()
{
	// I hope this is the right way to delete the handle returned by
	// vfs_load... C++ destructors would be far less ambiguous ;-)
	if (m_Handle)
		mem_free_h(m_Handle);
}

PSRETURN CVFSFile::Load(const char* filename, uint flags /* default 0 */)
{
	assert(!m_Handle && "Mustn't open files twice");
	if (m_Handle)
		throw PSERROR_CVFSFile_AlreadyLoaded();

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
	// Die in a very obvious way, to avoid problems caused by
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
	// Die in a very obvious way, to avoid subtle problems caused by
	// accidentally forgetting to check that the open succeeded
	if (!m_Handle)
	{
		debug_warn("GetBuffer() called with no file loaded");
		throw PSERROR_CVFSFile_InvalidBufferAccess();
	}

	return std::string((char*)m_Buffer, m_BufferSize);
}
