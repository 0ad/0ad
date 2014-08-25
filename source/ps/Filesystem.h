/* Copyright (C) 2014 Wildfire Games.
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

#ifndef INCLUDED_PS_FILESYSTEM
#define INCLUDED_PS_FILESYSTEM

#include "lib/file/file.h"
#include "lib/file/io/io.h"
#include "lib/file/io/write_buffer.h"
#include "lib/file/vfs/vfs_util.h"

#include "ps/CStr.h"
#include "ps/Errors.h"

extern PIVFS g_VFS;

extern bool VfsFileExists(const VfsPath& pathname);

/**
 * callback function type for file change notifications
 */
typedef Status (*FileReloadFunc)(void* param, const VfsPath& path);

/**
 * register a callback function to be called by ReloadChangedFiles
 */
void RegisterFileReloadFunc(FileReloadFunc func, void* obj);

/**
 * delete a callback function registered with RegisterFileReloadFunc
 * (removes any with the same func and obj)
 */
void UnregisterFileReloadFunc(FileReloadFunc func, void* obj);

/**
 * poll for directory change notifications and reload all affected files.
 * must be called regularly (e.g. once a frame), else notifications
 * may be lost.
 * note: polling is much simpler than asynchronous notifications.
 **/
extern Status ReloadChangedFiles();

/**
 * Helper function to handle API differences between Boost Filesystem v2 and v3
 */
std::wstring GetWstringFromWpath(const fs::wpath& path);

ERROR_GROUP(CVFSFile);
ERROR_TYPE(CVFSFile, LoadFailed);
ERROR_TYPE(CVFSFile, AlreadyLoaded);

/**
 * Reads a file, then gives read-only access to the contents
 */
class CVFSFile
{
public:
	CVFSFile();
	~CVFSFile();

	/**
	 * Returns either PSRETURN_OK or PSRETURN_CVFSFile_LoadFailed
	 * @note Dies if the file has already been successfully loaded
	 * @param log Whether to log a failure to load a file
	 */
	PSRETURN Load(const PIVFS& vfs, const VfsPath& filename, bool log = true);

	/**
	 * Returns buffer of this file as a stream of bytes
	 * @note file must have been successfully loaded
	 */
	const u8* GetBuffer() const;
	size_t GetBufferSize() const;

	/**
	 * Returns contents of file as a string
	 * @note file must have been successfully loaded
	 */
	CStr GetAsString() const;

	/**
	 * Returns contents of a UTF-8 encoded file as a string with optional BOM removed
	 * @note file must have been successfully loaded
	 */
	CStr DecodeUTF8() const;

private:
	shared_ptr<u8> m_Buffer;
	size_t m_BufferSize;
};

#endif	// #ifndef INCLUDED_PS_FILESYSTEM
