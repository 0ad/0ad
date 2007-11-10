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

struct ICodec;

namespace ERR
{
	const LibError UNKNOWN_FORMAT = -110400;
	const LibError IS_COMPRESSED  = -110401;
}

enum ArchiveEntryFlags
{
	// (this avoids having to interpret each archive's method values)
	AEF_COMPRESSED = 1,

	// indicates ArchiveEntry.ofs points to a "local file header" instead of
	// the file data. a fixup routine is called upon file open; it skips
	// past LFH and clears this flag.
	// this is somewhat of a hack, but vital to archive open performance.
	// without it, we'd have to scan through the entire archive file,
	// which can take *seconds*.
	// (we cannot use the information in CDFH, because its 'extra' field
	// has been observed to differ from that of the LFH)
	// by reading LFH when a file in archive is opened, the block cache
	// absorbs the IO cost because the file will likely be read anyway.
	AEF_NEEDS_FIXUP = 2
};

// holds all per-file information extracted from the header.
// this is intended to work for all archive types.
struct ArchiveEntry
{
	off_t ofs;
	size_t usize;
	size_t csize;
	time_t mtime;
	u32 checksum;
	uint method;
	uint flags;	// ArchiveEntryFlags

	// note that size == usize isn't foolproof, and adding a flag to
	// ofs or size is ugly and error-prone.
	bool IsCompressed() const
	{
		return (flags & AEF_COMPRESSED) != 0;
	}
};


// successively called for each archive entry.
typedef LibError (*ArchiveCB)(const char* pathname, const ArchiveEntry& ae, uintptr_t cbData);


struct IArchive
{
	IArchive(const char* pathname);
	virtual ~IArchive();

	virtual boost::shared_ptr<ICodec> CreateDecompressor() const = 0;

	/**
	 * call back for each file entry in the archive.
	 **/
	virtual LibError ForEachEntry(ArchiveCB cb, uintptr_t cbData) const = 0;

	virtual LibError LoadFile(ArchiveEntry& archiveEntry) const = 0;
};


// note: when creating an archive, any existing file with the given pathname
// will be overwritten.
struct IArchiveBuilder
{
	/**
	 * write out the archive to disk; only hereafter is it valid.
	 **/
	virtual ~IArchiveBuilder();

	virtual boost::shared_ptr<ICodec> CreateCompressor() const = 0;

	/**
	 * add a file to the archive.
	 *
	 * @param fileContents the file data; its compression method is defined by
	 * ae.method and can be CM_NONE.
	 **/
	virtual LibError AddFile(const ArchiveEntry& ae, const u8* fileContents) = 0;
};

#endif	// #ifndef INCLUDED_ARCHIVE
