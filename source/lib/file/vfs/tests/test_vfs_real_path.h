/* Copyright (C) 2021 Wildfire Games.
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

#include "lib/file/vfs/vfs.h"
#include "lib/file/vfs/vfs_populate.h"
#include "lib/os_path.h"

static OsPath TEST_FOLDER(DataDir()/"_test.temp");

extern PIVFS g_VFS;

class TestVfsPopulate : public CxxTest::TestSuite
{
	void initVfs()
	{
		g_VFS = CreateVfs();
	}

	void deinitVfs()
	{
		if(DirectoryExists(TEST_FOLDER))
			DeleteDirectory(TEST_FOLDER);

		g_VFS.reset();
	}

	void createRealDir(const OsPath& path)
	{
		if(DirectoryExists(path))
			DeleteDirectory(path);
		CreateDirectories(path, 0700, false);
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

	/**
	 * This is a regression test. Before priorities were used for real directories, things were... Rather undefined.
	 * The new spec is that the path is relative to the highest priority subdirectory in the path.
	 * The order remains undefined in case of equal priority.
	 * (see below for tests on that).
	 */
	void test_write_path_hijacking()
	{
		createRealDir(TEST_FOLDER / "cache" / "some_folder");
		createRealDir(TEST_FOLDER / "some_mod" / "cache" / "some_mod");

		shared_ptr<u8> buf(new u8(1));

		g_VFS->Mount(L"", TEST_FOLDER / "some_mod" / "", 0, 0);

		// Access the subfolder, creating subdirectories in the VFS.
		g_VFS->CreateFile(L"cache/some_mod/peek.txt", buf, 0);

		g_VFS->Mount(L"cache/", TEST_FOLDER / "cache" / "", 0, 1);

		OsPath realPath;
		g_VFS->GetDirectoryRealPath(L"cache/", realPath);
		TS_ASSERT_EQUALS(realPath, TEST_FOLDER / "cache" / "");
		g_VFS->CreateFile(L"cache/test.txt", buf, 0);
		g_VFS->GetRealPath(L"cache/test.txt", realPath);
		TS_ASSERT_EQUALS(realPath, TEST_FOLDER / "cache" / "test.txt");

		g_VFS->GetDirectoryRealPath(L"cache/some_mod/", realPath);
		TS_ASSERT_EQUALS(realPath, TEST_FOLDER / "cache" / "some_mod" / "");
		g_VFS->CreateFile(L"cache/some_mod/test.txt", buf, 0);
		g_VFS->GetRealPath(L"cache/some_mod/test.txt", realPath);
		TS_ASSERT_EQUALS(realPath, TEST_FOLDER / "cache" / "some_mod" / "test.txt");
	};

	void test_priority()
	{
		createRealDir(TEST_FOLDER / "mod_a");
		createRealDir(TEST_FOLDER / "mod_b");

		// Check that the real directory points to the highest priority mod.
		g_VFS->Mount(L"mod", TEST_FOLDER / "mod_a" / "", 0, 1);
		g_VFS->Mount(L"mod", TEST_FOLDER / "mod_b" / "", 0, 0);

		// For consistency, populate everything.
		ignore_result(g_VFS->TextRepresentation().c_str());

		OsPath realPath;
		g_VFS->GetDirectoryRealPath(L"mod", realPath);
		TS_ASSERT_EQUALS(realPath, TEST_FOLDER / "mod_a" / "");
	};

	void test_real_path_0()
	{
		createRealDir(TEST_FOLDER / "mod_0" / "folder_a");
		createRealDir(TEST_FOLDER / "mod_0" / "folder_a" / "subfolder");
		createRealDir(TEST_FOLDER / "mod_0" / "folder_b");
		createRealDir(TEST_FOLDER / "mod_1" / "folder_a");
		createRealDir(TEST_FOLDER / "folder_a");

		g_VFS->Mount(L"folder_a/", TEST_FOLDER / "folder_a" / "", 0, 2);
		g_VFS->Mount(L"", TEST_FOLDER / "mod_1" / "", 0, 1);
		g_VFS->Mount(L"", TEST_FOLDER / "mod_0" / "", 0, 0);

		OsPath realPath;
		g_VFS->GetDirectoryRealPath(L"folder_a/", realPath);
		TS_ASSERT_EQUALS(realPath, TEST_FOLDER / "folder_a" / "");
		// Despite being lower priority, we still load non-conflicting files of mod_0
		g_VFS->GetDirectoryRealPath(L"folder_b/", realPath);
		// However their real path is rewritten to mod_1.
		TS_ASSERT_EQUALS(realPath, TEST_FOLDER / "mod_1" / "folder_b" / "");

		// (including sub-subfolders)
		g_VFS->GetDirectoryRealPath(L"folder_a/subfolder/", realPath);
		// However their real path is rewritten.
		TS_ASSERT_EQUALS(realPath, TEST_FOLDER / "folder_a" / "subfolder" / "");
	};

	void test_real_path_1()
	{
		createRealDir(TEST_FOLDER / "mod_0" / "folder_a");
		createRealDir(TEST_FOLDER / "mod_1" / "folder_a");
		createRealDir(TEST_FOLDER / "folder_a");

		// Equal priority, the order is undetermined.
		OsPath realPath;
		g_VFS->Mount(L"", TEST_FOLDER / "mod_0" / "", 0, 1);
		g_VFS->Mount(L"folder_a/", TEST_FOLDER / "folder_a" / "", 0, 1);
		g_VFS->Mount(L"", TEST_FOLDER / "mod_1" / "", 0, 0);

		g_VFS->GetDirectoryRealPath(L"folder_a/", realPath);
		TS_ASSERT_EQUALS(realPath, TEST_FOLDER / "folder_a" / "");
	};

	// Same as test_real_path_1, but invert the mounting order
	// (may or may not result in different behaviour).
	void test_real_path_2()
	{
		createRealDir(TEST_FOLDER / "mod_0" / "folder_a");
		createRealDir(TEST_FOLDER / "mod_1" / "folder_a");
		createRealDir(TEST_FOLDER / "folder_a");

		// Equal priority, the order is undetermined.
		OsPath realPath;
		g_VFS->Mount(L"folder_a/", TEST_FOLDER / "folder_a" / "", 0, 1);
		g_VFS->Mount(L"", TEST_FOLDER / "mod_1" / "", 0, 0);
		g_VFS->Mount(L"", TEST_FOLDER / "mod_0" / "", 0, 1);

		g_VFS->GetDirectoryRealPath(L"folder_a/", realPath);
		TS_ASSERT_EQUALS(realPath, TEST_FOLDER / "folder_a" / "");
	};

	void test_real_path_subpath()
	{
		createRealDir(TEST_FOLDER / "mod_0" / "folder" / "subfolder");
		createRealDir(TEST_FOLDER / "mod_1" / "folder" / "subfolder");
		createRealDir(TEST_FOLDER / "other_folder");

		g_VFS->Mount(L"", TEST_FOLDER / "mod_0" / "", 0, 0);
		g_VFS->Mount(L"", TEST_FOLDER / "mod_1" / "", 0, 1);
		g_VFS->Mount(L"folder/subfolder/", TEST_FOLDER / "other_folder" / "", 0, 2);

		OsPath realPath;
		g_VFS->GetDirectoryRealPath(L"folder/subfolder/", realPath);
		TS_ASSERT_EQUALS(realPath, TEST_FOLDER / "other_folder" / "");
	};
};
