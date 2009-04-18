/**
 * =========================================================================
 * File        : file_system.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_FILE_SYSTEM
#define INCLUDED_FILE_SYSTEM

class FileInfo
{
public:
	FileInfo()
	{
	}

	FileInfo(const std::string& name, off_t size, time_t mtime)
		: m_name(name), m_size(size), m_mtime(mtime)
	{
	}

	const std::string& Name() const
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
	std::string m_name;
	off_t m_size;
	time_t m_mtime;
};

typedef std::vector<FileInfo> FileInfos;
typedef std::vector<std::string> DirectoryNames;

#endif	// #ifndef INCLUDED_FILE_SYSTEM
