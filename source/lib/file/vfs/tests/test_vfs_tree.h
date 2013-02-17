/* Copyright (c) 2013 Wildfire Games
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

#include "lib/file/vfs/vfs_tree.h"
#include "lib/file/common/file_loader.h"

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
};
