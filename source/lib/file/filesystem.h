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


/**
 * information describing file system entries (i.e. files or directories)
 *
 * note: don't be extravagant with memory - dir_ForEachSortedEntry allocates
 * one instance of this per directory entry.
 **/
class FileInfo
{
public:
	FileInfo()
	{
	}

	FileInfo(const char* name, off_t size, time_t mtime)
		: m_name(name), m_size(size), m_mtime(mtime)
	{
	}

	const char* Name() const
	{
		return m_name;
	}

	off_t Size() const
	{
		return m_size;
	}

	time_t MTime() const
	{
		return m_mtime;
	}

private:
	/**
	 * name of the entry; does not include a path.
	 * the underlying storage is guaranteed to remain valid and must not
	 * be freed/modified.
	 **/
	const char* m_name;
	off_t m_size;
	time_t m_mtime;
};


struct IFilesystem
{
	virtual ~IFilesystem();

	virtual LibError GetFileInfo(const char* pathname, FileInfo& fileInfo) const = 0;

	// note: this interface avoids having to lock a directory while an
	// iterator is extant.
	virtual LibError GetDirectoryEntries(const char* path, std::vector<FileInfo>* files, std::vector<const char*>* subdirectories) const = 0;
};

#endif	// #ifndef INCLUDED_FILESYSTEM
