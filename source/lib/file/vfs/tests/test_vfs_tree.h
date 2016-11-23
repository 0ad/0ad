/* Copyright (c) 2016 Wildfire Games
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

#include "lib/file/common/file_loader.h"
#include "lib/file/vfs/vfs.h"
#include "lib/file/vfs/vfs_lookup.h"
#include "lib/file/vfs/vfs_tree.h"

class MockLoader : public IFileLoader
{
private:
	size_t m_Precedence;
public:
	MockLoader(size_t precedence) :
		m_Precedence(precedence)
	{
	}

	size_t Precedence() const { return m_Precedence; }
	wchar_t LocationCode() const { return L'\0'; }
	OsPath Path() const { return L"";}
	Status Load(const OsPath& UNUSED(name), const shared_ptr<u8>& UNUSED(buf), size_t UNUSED(size)) const {return INFO::OK; }
};

class TestVfsTree : public CxxTest::TestSuite
{
	/**
	 * We create a few different "mods" here to test proper .DELETED
	 * behavior.
	 *
	 * To check which file is used we use the priority.
	 *
	 * 1
	 * +--a
	 * +--b/
	 * |  +--a
	 * |  \--b/
	 * |     \--a
	 * \--c/
	 *    +--a
	 *    \--b
	 *
	 * 2
	 * +--a.DELETED
	 * +--b/
	 * |  +--a
	 * |  \--b.DELETED
	 * +--c.DELETED
	 * \--c/
	 *    +--a
	 *    \--b
	 *
	 * 3
	 * +--a
	 * \--b/
	 *    \--b/
	 *       \--a
	 */
	void mount_mod(size_t mod, VfsDirectory& dir)
	{
		size_t priority = mod;
		PIFileLoader loader(new MockLoader(1));
		switch(mod)
		{
		case 1:
		{
			dir.AddFile(VfsFile("a", 0, 0, priority, loader));
			VfsDirectory* b = dir.AddSubdirectory("b");
				b->AddFile(VfsFile("a", 0, 0, priority, loader));
				VfsDirectory* b_b = b->AddSubdirectory("b");
					b_b->AddFile(VfsFile("a", 0, 0, priority, loader));
			VfsDirectory* c = dir.AddSubdirectory("c");
				c->AddFile(VfsFile("a", 0, 0, priority, loader));
				c->AddFile(VfsFile("b", 0, 0, priority, loader));
			break;
		}
		case 2:
		{
			dir.DeleteSubtree(VfsFile("a.DELETED", 0, 0, priority, loader));
			VfsDirectory* b = dir.AddSubdirectory("b");
				b->AddFile(VfsFile("a", 0, 0, priority, loader));
				b->DeleteSubtree(VfsFile("b.DELETED", 0, 0, priority, loader));
			dir.DeleteSubtree(VfsFile("c.DELETED", 0, 0, priority, loader));
			VfsDirectory* c = dir.AddSubdirectory("c");
				c->AddFile(VfsFile("a", 0, 0, priority, loader));
				c->AddFile(VfsFile("b", 0, 0, priority, loader));
			break;
		}
		case 3:
		{
			dir.AddFile(VfsFile("a", 0, 0, priority, loader));
			VfsDirectory* b = dir.AddSubdirectory("b");
				VfsDirectory* b_b = b->AddSubdirectory("b");
					b_b->AddFile(VfsFile("a", 0, 0, priority, loader));
			break;
		}
		NODEFAULT;
		}
	}

	void check_priority(VfsDirectory& root, const VfsPath& path, size_t priority)
	{
		VfsDirectory* dir; VfsFile* file;
		TS_ASSERT_OK(vfs_Lookup(path, &root, dir, &file, VFS_LOOKUP_SKIP_POPULATE));
		TS_ASSERT_EQUALS(file->Priority(), priority);
	}

	void file_does_not_exists(VfsDirectory& root, const VfsPath& path)
	{
		VfsDirectory* dir; VfsFile* file;
		TS_ASSERT_EQUALS(vfs_Lookup(path, &root, dir, &file, VFS_LOOKUP_SKIP_POPULATE), ERR::VFS_FILE_NOT_FOUND);
	}

	void directory_exists(VfsDirectory& root, const VfsPath& path, Status error = INFO::OK)
	{
		VfsDirectory* dir;
		TS_ASSERT_EQUALS(vfs_Lookup(path, &root, dir, nullptr, VFS_LOOKUP_SKIP_POPULATE), error);
	}

public:
	void test_replacement()
	{
		VfsDirectory dir;
		PIFileLoader loader(new MockLoader(1));

		VfsFile file0("a", 0,  0, 0, loader);
		VfsFile file1("a", 0, 20, 0, loader);
		VfsFile file2("a", 0, 10, 1, loader);

		// Modification time
		TS_ASSERT_EQUALS(dir.AddFile(file0)->MTime(), file0.MTime());
		TS_ASSERT_EQUALS(dir.AddFile(file1)->MTime(), file1.MTime());
		TS_ASSERT_EQUALS(dir.AddFile(file0)->MTime(), file1.MTime());

		// Priority
		TS_ASSERT_EQUALS(dir.AddFile(file2)->MTime(), file2.MTime());
		TS_ASSERT_EQUALS(dir.AddFile(file1)->MTime(), file2.MTime());
	}

	void test_deleted()
	{
		VfsDirectory dir;

		mount_mod(1, dir);
		mount_mod(2, dir);
		file_does_not_exists(dir, "a");
		check_priority(dir, "b/a", 2);
		directory_exists(dir, "b/b/", ERR::VFS_DIR_NOT_FOUND);
		directory_exists(dir, "c/");
		check_priority(dir, "c/a", 2);
		check_priority(dir, "c/b", 2);
		dir.Clear();


		mount_mod(1, dir);
		mount_mod(2, dir);
		mount_mod(3, dir);
		check_priority(dir, "a", 3);
		check_priority(dir, "b/b/a", 3);
		dir.Clear();


		mount_mod(1, dir);
		mount_mod(3, dir);
		mount_mod(2, dir);
		check_priority(dir, "a", 3);
		check_priority(dir, "b/b/a", 3);
		dir.Clear();
	}
};
