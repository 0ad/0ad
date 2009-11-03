/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"
#include "Filesystem.h"

#include "ps/CLogger.h"

#define LOG_CATEGORY L"file"


PIVFS g_VFS;

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
		LOG(CLogger::Error, LOG_CATEGORY, L"CVFSFile: file %ls couldn't be opened (vfs_load: %ld)", filename.string().c_str(), ret);
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
		debug_warn(L"GetBuffer() called with no file loaded");
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
