/* Copyright (C) 2019 Wildfire Games.
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

#include "lib/self_test.h"

#include "lib/file/vfs/vfs_util.h"
#include "lib/file/file_system.h"
#include "lib/os_path.h"

static OsPath MOD_PATH(DataDir()/"mods"/"_test.vfs");
static OsPath CACHE_PATH(DataDir()/"_testcache");

extern PIVFS g_VFS;

class TestVfsUtil : public CxxTest::TestSuite
{
	void initVfs()
	{
		// Initialise VFS:

		// Make sure the required directories doesn't exist when we start,
		// in case the previous test aborted and left them full of junk
		if(DirectoryExists(MOD_PATH))
			DeleteDirectory(MOD_PATH);
		if(DirectoryExists(CACHE_PATH))
			DeleteDirectory(CACHE_PATH);

		g_VFS = CreateVfs();

		TS_ASSERT_OK(g_VFS->Mount(L"", MOD_PATH));

		// Mount _testcache onto virtual /cache - don't use the normal cache
		// directory because that's full of loads of cached files from the
		// proper game and takes a long time to load.
		TS_ASSERT_OK(g_VFS->Mount(L"cache/", CACHE_PATH));
	}

	void deinitVfs()
	{
		g_VFS.reset();
		DeleteDirectory(MOD_PATH);
		DeleteDirectory(CACHE_PATH);
	}

public:
	void setUp()
	{
		initVfs();
	}

	void tearDown()
	{
		deinitVfs();
	}

	void test_getPathnames()
	{
		std::shared_ptr<u8> nodata(new u8);
		g_VFS->CreateFile("test_file.txt", nodata, 0);
		g_VFS->CreateFile("test_file2.txt", nodata, 0);
		g_VFS->CreateFile("test_file3.txt", nodata, 0);
		g_VFS->CreateFile("test_file2.not_txt", nodata, 0);
		g_VFS->CreateFile("sub_folder_a/sub_test_file1.txt", nodata, 0);
		g_VFS->CreateFile("sub_folder_a/sub_test_file2.txt", nodata, 0);
		g_VFS->CreateFile("sub_folder_b/sub_test_file1.txt", nodata, 0);
		g_VFS->CreateFile("sub_folder_b/another_file.not_txt", nodata, 0);

		VfsPaths pathNames;
		vfs::GetPathnames(g_VFS, "", L"*.txt", pathNames);
		TS_ASSERT_EQUALS(pathNames.size(), 3);
		vfs::GetPathnames(g_VFS, "sub_folder_a/", L"*.txt", pathNames);
		TS_ASSERT_EQUALS(pathNames.size(), 5);
		vfs::GetPathnames(g_VFS, "sub_folder_b/", L"*.txt", pathNames);
		TS_ASSERT_EQUALS(pathNames.size(), 6);
	};
};
