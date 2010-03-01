/* Copyright (c) 2010 Wildfire Games
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

#include "precompiled.h"
#include "lib/file/common/real_directory.h"

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
	wtruncate(pathname.string().c_str(), size);

	return INFO::OK;
}


void RealDirectory::Watch()
{
	if(!m_watch)
		(void)dir_watch_Add(m_path, m_watch);
}


PRealDirectory CreateRealSubdirectory(const PRealDirectory& realDirectory, const std::wstring& subdirectoryName)
{
	const fs::wpath path = AddSlash(realDirectory->Path()/subdirectoryName);
	return PRealDirectory(new RealDirectory(path, realDirectory->Priority(), realDirectory->Flags()));
}
