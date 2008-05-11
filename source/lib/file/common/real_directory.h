#ifndef INCLUDED_REAL_DIRECTORY
#define INCLUDED_REAL_DIRECTORY

#include "file_loader.h"
#include "lib/file/path.h"

class RealDirectory : public IFileLoader
{
public:
	RealDirectory(const Path& path, size_t priority, int flags);

	const Path& GetPath() const
	{
		return m_path;
	}

	size_t Priority() const
	{
		return m_priority;
	}

	int Flags() const
	{
		return m_flags;
	}

	// IFileLoader
	virtual size_t Precedence() const;
	virtual char LocationCode() const;
	virtual LibError Load(const std::string& name, shared_ptr<u8> buf, size_t size) const;

	LibError Store(const std::string& name, shared_ptr<u8> fileContents, size_t size);

	void Watch();

private:
	RealDirectory(const RealDirectory& rhs);	// noncopyable due to const members
	RealDirectory& operator=(const RealDirectory& rhs);

	// note: paths are relative to the root directory, so storing the
	// entire path instead of just the portion relative to the mount point
	// is not all too wasteful.
	const Path m_path;

	const size_t m_priority;

	const int m_flags;

	// note: watches are needed in each directory because some APIs
	// (e.g. FAM) cannot watch entire trees with one call.
	void* m_watch;
};

typedef shared_ptr<RealDirectory> PRealDirectory;

extern PRealDirectory CreateRealSubdirectory(PRealDirectory realDirectory, const std::string& subdirectoryName);

#endif	// #ifndef INCLUDED_REAL_DIRECTORY
