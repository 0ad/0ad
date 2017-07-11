/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
	const Status ARCHIVE_UNKNOWN_FORMAT = -110400;
	const Status ARCHIVE_UNKNOWN_METHOD = -110401;
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
	typedef void (*ArchiveEntryCallback)(const VfsPath& pathname, const CFileInfo& fileInfo, PIArchiveFile archiveFile, uintptr_t cbData);
	virtual Status ReadEntries(ArchiveEntryCallback cb, uintptr_t cbData) = 0;
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
	 *
	 * @param pathname the actual file to add
	 * @param pathnameInArchive the name to store in the archive
	 **/
	virtual Status AddFile(const OsPath& pathname, const Path& pathnameInArchive) = 0;

	/**
	 * add a file to the archive, when it is already in memory and not on disk.
	 *
	 * @param data the uncompressed file contents to add
	 * @param size the length of data
	 * @param mtime the last-modified-time to be stored in the archive
	 * @param pathnameInArchive the name to store in the archive
	 **/
	virtual Status AddMemory(const u8* data, size_t size, time_t mtime, const OsPath& pathnameInArchive) = 0;
};

typedef shared_ptr<IArchiveWriter> PIArchiveWriter;

#endif	// #ifndef INCLUDED_ARCHIVE
