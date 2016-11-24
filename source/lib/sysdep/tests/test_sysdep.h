/* Copyright (c) 2014 Wildfire Games
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

#include "lib/lib.h"
#include "lib/secure_crt.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/filesystem.h"
#include "lib/sysdep/sysdep.h"

#if OS_BSD || OS_LINUX
# include "lib/sysdep/os/unix/unix_executable_pathname.h"
# include "mocks/dlfcn.h"
# include "mocks/unistd.h"
#endif

class TestSysdep : public CxxTest::TestSuite
{
public:
	void test_random()
	{
		u64 a = 0, b = 0;
		TS_ASSERT_OK(sys_generate_random_bytes((u8*)&a, sizeof(a)));
		TS_ASSERT_OK(sys_generate_random_bytes((u8*)&b, sizeof(b)));
		TS_ASSERT_DIFFERS(a, b);
	}

	void test_sys_ExecutablePathname()
	{
		OsPath path = sys_ExecutablePathname();

		// Try it first with the real executable (i.e. the
		// one that's running this test code)
		// Check it's absolute
		TSM_ASSERT(L"Path: "+path.string(), path_is_absolute(path.string().c_str()));
		// Check the file exists
		struct stat s;
		TSM_ASSERT_EQUALS(L"Path: "+path.string(), wstat(path, &s), 0);
	}

	void test_unix_ExecutablePathname()
	{
#if !(OS_BSD || OS_LINUX)
	}
#else
		// Since the implementation uses realpath, the tested files need to
		// really exist. So set up a directory tree for testing:

		const char* tmpdir = getenv("TMPDIR");
		if (! tmpdir) tmpdir = P_tmpdir;

		char root[PATH_MAX];
		sprintf_s(root, ARRAY_SIZE(root), "%s/pyrogenesis-test-sysdep-XXXXXX", tmpdir);
		TS_ASSERT(mkdtemp(root));

		char rootres[PATH_MAX];
		TS_ASSERT(realpath(root, rootres));

		std::string rootstr(rootres);
		OsPath rootstrw(rootstr);

		const char* dirs[] = {
			"/example",
			"/example/a",
			"/example/a/b",
			"/example/a/b/c",
			"/example/a/b/d",
			"/example/a/e",
			"/example/a/f"
		};
		const char* files[] = {
			"/example/executable",
			"/example/a/f/executable",
		};
		for (size_t i = 0; i < ARRAY_SIZE(dirs); ++i)
		{
			std::string name = rootstr + dirs[i];
			TS_ASSERT_EQUALS(mkdir(name.c_str(), 0700), 0);
		}
		for (size_t i = 0; i < ARRAY_SIZE(files); ++i)
		{
			std::string name = rootstr + files[i];
			FILE* f = fopen(name.c_str(), "w");
			TS_ASSERT(f);
			fclose(f);
		}

		// Try with absolute paths
		{
			Mock_dladdr d(rootstr+"/example/executable");
			TS_ASSERT_PATH_EQUALS(unix_ExecutablePathname(), rootstrw/L"example/executable");
		}
		{
			Mock_dladdr d(rootstr+"/example/./a/b/../e/../../executable");
			TS_ASSERT_PATH_EQUALS(unix_ExecutablePathname(), rootstrw/L"example/executable");
		}

		// Try with relative paths
		{
			Mock_dladdr d("./executable");
			Mock_getcwd m(rootstr+"/example");
			TS_ASSERT_PATH_EQUALS(unix_ExecutablePathname(), rootstrw/L"example/executable");
		}
		{
			Mock_dladdr d("./executable");
			Mock_getcwd m(rootstr+"/example/");
			TS_ASSERT_PATH_EQUALS(unix_ExecutablePathname(), rootstrw/L"example/executable");
		}
		{
			Mock_dladdr d("../d/../../f/executable");
			Mock_getcwd m(rootstr+"/example/a/b/c");
			TS_ASSERT_PATH_EQUALS(unix_ExecutablePathname(), rootstrw/L"example/a/f/executable");
		}

		// Try with pathless names
		{
			Mock_dladdr d("executable");
			TS_ASSERT_PATH_EQUALS(unix_ExecutablePathname(), OsPath());
		}

		// Clean up the temporary files
		for (size_t i = 0; i < ARRAY_SIZE(files); ++i)
		{
			std::string name = rootstr + files[i];
			TS_ASSERT_EQUALS(unlink(name.c_str()), 0);
		}
		for (ssize_t i = ARRAY_SIZE(dirs)-1; i >= 0; --i) // reverse order
		{
			std::string name(root);
			name += dirs[i];
			TS_ASSERT_EQUALS(rmdir(name.c_str()), 0);
		}
		TS_ASSERT_EQUALS(rmdir(root), 0);
	}

	// Mock classes for test_unix_ExecutablePathname
	class Mock_dladdr : public T::Base_dladdr
	{
	public:
		Mock_dladdr(const std::string& fname) : fname_(fname) { }
		int dladdr(void *UNUSED(addr), Dl_info *info) {
			info->dli_fname = fname_.c_str();
			return 1;
		}
	private:
		std::string fname_;
	};

	class Mock_getcwd : public T::Base_getcwd
	{
	public:
		Mock_getcwd(const std::string& buf) : buf_(buf) { }
		char* getcwd(char* buf, size_t size) {
			strncpy_s(buf, size, buf_.c_str(), buf_.length());
			return buf;
		}
	private:
		std::string buf_;
	};
#endif // !(OS_BSD || OS_LINUX)

private:
	bool path_is_absolute(const wchar_t* path)
	{
		// UNIX-style absolute paths
		if (path[0] == '/')
			return true;

		// Windows UNC absolute paths
		if (path[0] == '\\' && path[1] == '\\')
			return true;

		// Windows drive-letter absolute paths
		if (iswalpha(path[0]) && path[1] == ':' && (path[2] == '/' || path[2] == '\\'))
			return true;

		return false;
	}

};
