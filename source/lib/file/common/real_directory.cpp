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

#include "precompiled.h"
#include "real_directory.h"

#include "lib/path_util.h"
#include "lib/file/file.h"
#include "lib/file/io/io.h"


RealDirectory::RealDirectory(const fs::wpath& path, size_t priority, size_t flags)
	: m_path(path), m_priority(priority), m_flags(flags)
{
}


/*virtual*/ size_t RealDirectory::Precedence() const
{
	return 1u;
}


/*virtual*/ wchar_t RealDirectory::LocationCode() const
{
	return 'F';
}


/*virtual*/ LibError RealDirectory::Load(const std::wstring& name, const shared_ptr<u8>& buf, size_t size) const
{
	const fs::wpath pathname(m_path/name);

	PFile file(new File);
	RETURN_ERR(file->Open(pathname, 'r'));

	RETURN_ERR(io_ReadAligned(file, 0, buf.get(), size));
	return INFO::OK;
}


LibError RealDirectory::Store(const std::wstring& name, const shared_ptr<u8>& fileContents, size_t size)
{
	const fs::wpath pathname(m_path/name);

	{
		PFile file(new File);
		RETURN_ERR(file->Open(pathname, 'w'));
		RETURN_ERR(io_WriteAligned(file, 0, fileContents.get(), size));
	}

	// io_WriteAligned pads the file; we need to truncate it to the actual
	// length. ftruncate can't be used because Windows' FILE_FLAG_NO_BUFFERING
	// only allows resizing to sector boundaries, so the file must first
	// be closed.
	const fs::path pathname_c(path_from_wpath(pathname));
	truncate(pathname_c.string().c_str(), size);

	return INFO::OK;
}


void RealDirectory::Watch()
{
	(void)dir_watch_Add(m_path, m_watch);
}


PRealDirectory CreateRealSubdirectory(const PRealDirectory& realDirectory, const std::wstring& subdirectoryName)
{
	const fs::wpath path(realDirectory->Path()/subdirectoryName);
	return PRealDirectory(new RealDirectory(path, realDirectory->Priority(), realDirectory->Flags()));
}
