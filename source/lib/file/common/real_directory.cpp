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

#include "lib/sysdep/filesystem.h"
#include "lib/file/file.h"
#include "lib/file/io/io.h"


RealDirectory::RealDirectory(const OsPath& path, size_t priority, size_t flags)
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


/*virtual*/ Status RealDirectory::Load(const OsPath& name, const shared_ptr<u8>& buf, size_t size) const
{
	return io::Load(m_path / name, buf.get(), size);
}


Status RealDirectory::Store(const OsPath& name, const shared_ptr<u8>& fileContents, size_t size)
{
	return io::Store(m_path / name, fileContents.get(), size);
}


void RealDirectory::Watch()
{
	if(!m_watch)
		(void)dir_watch_Add(m_path, m_watch);
}


PRealDirectory CreateRealSubdirectory(const PRealDirectory& realDirectory, const OsPath& subdirectoryName)
{
	const OsPath path = realDirectory->Path() / subdirectoryName/"";
	return PRealDirectory(new RealDirectory(path, realDirectory->Priority(), realDirectory->Flags()));
}
