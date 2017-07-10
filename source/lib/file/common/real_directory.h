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

#ifndef INCLUDED_REAL_DIRECTORY
#define INCLUDED_REAL_DIRECTORY

#include "lib/file/common/file_loader.h"
#include "lib/sysdep/dir_watch.h"

class RealDirectory : public IFileLoader
{
	NONCOPYABLE(RealDirectory);
public:
	RealDirectory(const OsPath& path, size_t priority, size_t flags);

	size_t Priority() const
	{
		return m_priority;
	}

	size_t Flags() const
	{
		return m_flags;
	}

	// IFileLoader
	virtual size_t Precedence() const;
	virtual wchar_t LocationCode() const;
	virtual OsPath Path() const
	{
		return m_path;
	}
	virtual Status Load(const OsPath& name, const shared_ptr<u8>& buf, size_t size) const;

	Status Store(const OsPath& name, const shared_ptr<u8>& fileContents, size_t size);

	void Watch();

private:
	// note: paths are relative to the root directory, so storing the
	// entire path instead of just the portion relative to the mount point
	// is not all too wasteful.
	const OsPath m_path;

	const size_t m_priority;

	const size_t m_flags;

	// note: watches are needed in each directory because some APIs
	// (e.g. FAM) cannot watch entire trees with one call.
	PDirWatch m_watch;
};

typedef shared_ptr<RealDirectory> PRealDirectory;

extern PRealDirectory CreateRealSubdirectory(const PRealDirectory& realDirectory, const OsPath& subdirectoryName);

#endif	// #ifndef INCLUDED_REAL_DIRECTORY
