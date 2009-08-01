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

#ifndef INCLUDED_REAL_DIRECTORY
#define INCLUDED_REAL_DIRECTORY

#include "file_loader.h"

class RealDirectory : public IFileLoader
{
public:
	RealDirectory(const fs::path& path, size_t priority, size_t flags);

	const fs::path& Path() const
	{
		return m_path;
	}

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
	virtual char LocationCode() const;
	virtual LibError Load(const std::string& name, const shared_ptr<u8>& buf, size_t size) const;

	LibError Store(const std::string& name, const shared_ptr<u8>& fileContents, size_t size);

	void Watch();

private:
	RealDirectory(const RealDirectory& rhs);	// noncopyable due to const members
	RealDirectory& operator=(const RealDirectory& rhs);

	// note: paths are relative to the root directory, so storing the
	// entire path instead of just the portion relative to the mount point
	// is not all too wasteful.
	const fs::path m_path;

	const size_t m_priority;

	const size_t m_flags;

	// note: watches are needed in each directory because some APIs
	// (e.g. FAM) cannot watch entire trees with one call.
	void* m_watch;
};

typedef shared_ptr<RealDirectory> PRealDirectory;

extern PRealDirectory CreateRealSubdirectory(const PRealDirectory& realDirectory, const std::string& subdirectoryName);

#endif	// #ifndef INCLUDED_REAL_DIRECTORY
