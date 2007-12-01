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

typedef std::vector<FileInfo> FileInfos;

typedef std::vector<const char*> Directories;

// 
struct IFileProvider
{
	virtual ~IFileProvider();

	virtual unsigned Precedence() const = 0;
	virtual char LocationCode() const = 0;

	virtual LibError Load(const char* name, const void* location, u8* fileContents, size_t size) const = 0;
	virtual LibError Store(const char* name, const void* location, const u8* fileContents, size_t size) const = 0;
};

typedef boost::shared_ptr<IFileProvider> PIFileProvider;


struct IFilesystem
{
	virtual ~IFilesystem();

	virtual LibError GetFileInfo(const char* pathname, FileInfo& fileInfo) const = 0;

	// note: this interface avoids having to lock a directory while an
	// iterator is extant.
	// (don't split this into 2 functions because POSIX can't implement
	// that efficiently)
	virtual LibError GetDirectoryEntries(const char* path, FileInfos* files, Directories* subdirectories) const = 0;
};

#endif	// #ifndef INCLUDED_FILESYSTEM
