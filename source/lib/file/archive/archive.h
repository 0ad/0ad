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

/*
 * interface for reading from and creating archives.
 */

#ifndef INCLUDED_ARCHIVE
#define INCLUDED_ARCHIVE

#include "lib/file/file_system.h"	// FileInfo
#include "lib/file/common/file_loader.h"
#include "lib/file/vfs/vfs_path.h"

// rationale: this module doesn't build a directory tree of the entries
// within an archive. that task is left to the VFS; here, we are only
// concerned with enumerating all archive entries.

namespace ERR
{
	const LibError ARCHIVE_UNKNOWN_FORMAT = -110400;
	const LibError ARCHIVE_UNKNOWN_METHOD = -110401;
}

struct IArchiveFile : public IFileLoader
{
};

typedef shared_ptr<IArchiveFile> PIArchiveFile;

struct IArchiveReader
{
	virtual ~IArchiveReader();

	/**
	 * called for each archive entry.
	 * @param pathname full pathname of entry; only valid during the callback.
	 **/
	typedef void (*ArchiveEntryCallback)(const VfsPath& pathname, const FileInfo& fileInfo, PIArchiveFile archiveFile, uintptr_t cbData);
	virtual LibError ReadEntries(ArchiveEntryCallback cb, uintptr_t cbData) = 0;
};

typedef shared_ptr<IArchiveReader> PIArchiveReader;

// note: when creating an archive, any existing file with the given pathname
// will be overwritten.

// rationale: don't support partial adding, i.e. updating archive with
// only one file. this would require overwriting parts of the Zip archive,
// which is annoying and slow. also, archives are usually built in
// seek-optimal order, which would break if we start inserting files.
// while testing, loose files can be used, so there's no loss.

struct IArchiveWriter
{
	/**
	 * write out the archive to disk; only hereafter is it valid.
	 **/
	virtual ~IArchiveWriter();

	/**
	 * add a file to the archive.
	 *
	 * rationale: passing in a filename instead of the compressed file
	 * contents makes for better encapsulation because callers don't need
	 * to know about the codec. one disadvantage is that loading the file
	 * contents can no longer take advantage of the VFS cache nor previously
	 * archived versions. however, the archive builder usually adds files
	 * precisely because they aren't in archives, and the cache would
	 * thrash anyway, so this is deemed acceptable.
	 **/
	virtual LibError AddFile(const fs::wpath& pathname) = 0;
};

typedef shared_ptr<IArchiveWriter> PIArchiveWriter;

#endif	// #ifndef INCLUDED_ARCHIVE
