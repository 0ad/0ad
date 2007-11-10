/**
 * =========================================================================
 * File        : filesystem.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_FILESYSTEM
#define INCLUDED_FILESYSTEM

#include <boost/shared_ptr.hpp>

#include "io/io_manager.h"	// IoCallback

namespace ERR
{
	const LibError FILE_ACCESS     = -110000;
	const LibError DIR_NOT_FOUND   = -110002;
	const LibError DIR_END         = -110003;
}


/**
 * information describing filesystem entries (i.e. files or directories)
 *
 * note: don't be extravagant with memory - dir_ForEachSortedEntry allocates
 * one instance of this per directory entry.
 **/
struct FilesystemEntry
{
	off_t size;
	time_t mtime;

	/**
	 * name of the entry; does not include a path.
	 * the underlying storage is guaranteed to remain valid and must not
	 * be freed/modified.
	 **/
	const char* name;

	// only defined for VFS files; points to their TMount.
	const void* mount;

	bool IsDirectory() const
	{
		return (size == -1);
	}
};


// (note: this is defined in the public header to promote inlining of the
// DirectoryIterator (concrete class) member functions.)
// documentation: see DirectoryIterator
struct IDirectoryIterator
{
	virtual ~IDirectoryIterator();
	virtual LibError NextEntry(FilesystemEntry& fsEntry) = 0;
};


/**
 * flags controlling file IO and caching behavior.
 **/
enum FileFlags
{
	// translate newlines: convert from/to native representation when
	// reading/writing. this is useful if files we create need to be
	// edited externally - e.g. Notepad requires \r\n.
	// caveats:
	// - FILE_NO_AIO must be set; translation is done by OS read()/write().
	// - not supported by POSIX, so this currently only has meaning on Win32.
	FILE_TEXT = 0x01,

	// skip the aio path and use the OS-provided synchronous blocking
	// read()/write() calls. this avoids the need for buffer alignment
	// set out below, so it's useful for writing small text files.
	// note: despite its more heavyweight operation, aio is still
	// worthwhile for small files, so it is not automatically disabled.
	FILE_NO_AIO = 0x02,

	// do not add the (entire) contents of this file to the cache.
	// this flag should be specified when the data is cached at a higher
	// level (e.g. OpenGL textures) to avoid wasting precious cache space.
	FILE_NO_CACHE = 0x04,

	// enable caching individual blocks read from a file. the block cache
	// is small, organized as LRU and incurs some copying overhead, so it
	// should only be enabled when needed. this is the case for archives,
	// where the cache absorbs overhead of block-aligning all IOs.
	FILE_CACHE_BLOCK = 0x08,

	// instruct file_open not to set FileCommon.atom_fn.
	// this is a slight optimization used by VFS code: file_open
	// would store the portable name, which is only used when calling
	// the OS's open(); this would unnecessarily waste atom_fn memory.
	//
	// note: other file.cpp functions require atom_fn to be set,
	// so this behavior is only triggered via flag (caller is
	// promising they will set atom_fn).
	FILE_DONT_SET_FN = 0x20,

	// (only relevant for VFS) file will be written into the
	// appropriate subdirectory of the mount point established by
	// vfs_set_write_target. see documentation there.
	FILE_WRITE_TO_TARGET = 0x40,

	// sum of all flags above. used when validating flag parameters.
	FILE_FLAG_ALL = 0x7F
};


struct IFilesystem
{
	virtual ~IFilesystem();

	/**
	 * @return a single character identifying the filesystem.
	 *
	 * this is useful for VFS directory listings, where an indication is
	 * made of where the file is actually stored.
	 **/
	virtual char IdentificationCode() const = 0;

	/**
	 * @return a number that represents the precedence of this filesystem.
	 *
	 * when mounting into the VFS, entries from a filesystem with higher
	 * precedence override otherwise equivalent files.
	 **/
	virtual int Precedence() const = 0;

	virtual LibError GetEntry(const char* pathname, FilesystemEntry& fsEntry) const = 0;

	virtual LibError CreateDirectory(const char* dirPath) = 0;
	virtual LibError DeleteDirectory(const char* dirPath) = 0;
	virtual IDirectoryIterator* OpenDirectory(const char* dirPath) const = 0;

	// note: only allowing either reads or writes simplifies file cache
	// coherency (need only invalidate when closing a FILE_WRITE file).
	virtual LibError CreateFile(const char* pathname, const u8* buf, size_t size, uint flags = 0) = 0;
	virtual LibError DeleteFile(const char* pathname) = 0;

	// read the entire file.
	// return number of bytes transferred (see above), or a negative error code.
	//
	// if non-NULL, <cb> is called for each block transferred, passing <cbData>.
	// it returns how much data was actually transferred, or a negative error
	// code (in which case we abort the transfer and return that value).
	// the callback mechanism is useful for user progress notification or
	// processing data while waiting for the next I/O to complete
	// (quasi-parallel, without the complexity of threads).
	virtual LibError LoadFile(const char* pathname, const u8*& buf, size_t size, uint flags = 0, IoCallback cb = 0, uintptr_t cbData = 0) = 0;
};



/**
 * (mostly) insulating concrete class providing iterator access to
 * directory entries.
 * this is usable for posix, VFS, etc.; instances are created via IFilesystem.
 **/
class DirectoryIterator
{
public:
	DirectoryIterator(IFilesystem* fs, const char* dirPath)
		: m_impl(fs->OpenDirectory(dirPath))
	{
	}

	// return ERR::DIR_END if all entries have already been returned once,
	// another negative error code, or INFO::OK on success, in which case <fsEntry>
	// describes the next (order is unspecified) directory entry.
	LibError NextEntry(FilesystemEntry& fsEntry)
	{
		return m_impl.get()->NextEntry(fsEntry);
	}

private:
	boost::shared_ptr<IDirectoryIterator> m_impl;
};

#endif	// #ifndef INCLUDED_FILESYSTEM
