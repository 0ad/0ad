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

#include "lib/self_test.h"

#include "ps/XML/Xeromyces.h"
#include "lib/file/vfs/vfs.h"
#include "lib/sysdep/sysdep.h"

// FIXME: copied from test_MeshManager
static fs::wpath DataDir()
{
	fs::wpath path;
	TS_ASSERT_OK(sys_get_executable_name(path));
	return path.branch_path()/L"../data";
}

class TestXeromyces : public CxxTest::TestSuite 
{
public:
	void test_paths()
	{
		PIVFS vfs = CreateVfs(20*MiB);

		TS_ASSERT_OK(vfs->Mount(L"", DataDir()/L"mods/_test.xero"));

		VfsPath xmbPath;

		CXeromyces::GetXMBPath(vfs, L"test1.xml", L"test1.xmb", xmbPath);
		TS_ASSERT_WSTR_EQUALS(xmbPath.string(), L"cache/mods/_test.xero/xmb/test1.xmb");

		CXeromyces::GetXMBPath(vfs, L"a/b/test1.xml", L"a/b/test1.xmb", xmbPath);
		TS_ASSERT_WSTR_EQUALS(xmbPath.string(), L"cache/mods/_test.xero/xmb/a/b/test1.xmb");
	}

	// TODO: Should test the reading/parsing/writing code,
	// and parse error handling
};
