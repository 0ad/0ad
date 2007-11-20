/**
 * =========================================================================
 * File        : archive.h
 * Project     : 0 A.D.
 * Description : interface for reading from and creating archives.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_ARCHIVE
#define INCLUDED_ARCHIVE

// rationale: this module doesn't build a directory tree of the entries
// within an archive. that task is left to the VFS; here, we are only
// concerned with enumerating all archive entries.

namespace ERR
{
	const LibError ARCHIVE_UNKNOWN_FORMAT = -110400;
	const LibError ARCHIVE_UNKNOWN_METHOD = -110401;
}

// opaque 'memento' of an archive entry. the instances are stored by
// the IArchiveReader implementation.
struct ArchiveEntry;

struct IArchiveReader
{
	virtual ~IArchiveReader();

	/**
	 * called for each archive entry.
	 * @param pathname full pathname of entry (unique pointer)
	 **/
	typedef void (*ArchiveEntryCallback)(const char* pathname, const ArchiveEntry& archiveEntry, uintptr_t cbData);
	virtual LibError ReadEntries(ArchiveEntryCallback cb, uintptr_t cbData) = 0;

	virtual LibError LoadFile(ArchiveEntry& archiveEntry, u8* fileContents) const = 0;
};

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
	virtual LibError AddFile(const char* pathname) = 0;
};

/**
 * describes an archive entry (either file or directory).
 **/
struct ArchiveEntry
{
	ArchiveEntry()
	{
	}

	ArchiveEntry(off_t ofs, off_t usize, off_t csize, time_t mtime, u32 checksum, uint method, uint flags)
	{
		this->ofs = ofs;
		this->usize = usize;
		this->csize = csize;
		this->mtime = mtime;
		this->checksum = checksum;
		this->method = method;
		this->flags = flags;
	}

	// this is needed by the VFS and stored here instead of in VfsFile to
	// yield a nice 32-byte size.
	IArchiveReader* archiveReader;

	off_t ofs;
	off_t usize;
	off_t csize;
	time_t mtime;

	// the archive implementation establishes the meaning of the following:

	u32 checksum;
	uint method;
	uint flags;
};

#endif	// #ifndef INCLUDED_ARCHIVE
