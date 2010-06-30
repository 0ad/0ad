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

#include "lib/self_test.h"

#include "lib/lib.h"
#include "lib/path_util.h"
#include "lib/secure_crt.h"
#include "lib/utf8.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/sysdep.h"
#include "lib/posix/posix.h"	// fminf etc.

#if OS_LINUX
# include "mocks/dlfcn.h"
# include "mocks/unistd.h"
#endif

#include <cxxtest/PsTestWrapper.h>

class TestSysdep : public CxxTest::TestSuite 
{
public:
	void test_float_int()
	{
		TS_ASSERT_EQUALS(cpu_i32FromFloat(0.99999f), 0);
		TS_ASSERT_EQUALS(cpu_i32FromFloat(1.0f), 1);
		TS_ASSERT_EQUALS(cpu_i32FromFloat(1.01f), 1);
		TS_ASSERT_EQUALS(cpu_i32FromFloat(5.6f), 5);

		TS_ASSERT_EQUALS(cpu_i32FromDouble(0.99999), 0);
		TS_ASSERT_EQUALS(cpu_i32FromDouble(1.0), 1);
		TS_ASSERT_EQUALS(cpu_i32FromDouble(1.01), 1);
		TS_ASSERT_EQUALS(cpu_i32FromDouble(5.6), 5);

		TS_ASSERT_EQUALS(cpu_i64FromDouble(0.99999), 0LL);
		TS_ASSERT_EQUALS(cpu_i64FromDouble(1.0), 1LL);
		TS_ASSERT_EQUALS(cpu_i64FromDouble(1.01), 1LL);
		TS_ASSERT_EQUALS(cpu_i64FromDouble(5.6), 5LL);
	}

	void test_round()
	{
		TS_ASSERT_EQUALS(rintf(0.99999f), 1.0f);
		TS_ASSERT_EQUALS(rintf(1.0f), 1.0f);
		TS_ASSERT_EQUALS(rintf(1.01f), 1.0f);
		TS_ASSERT_EQUALS(rintf(5.6f), 6.0f);

		TS_ASSERT_EQUALS(rint(0.99999), 1.0);
		TS_ASSERT_EQUALS(rint(1.0), 1.0);
		TS_ASSERT_EQUALS(rint(1.01), 1.0);
		TS_ASSERT_EQUALS(rint(5.6), 6.0);
	}

	void test_min_max()
	{
		TS_ASSERT_EQUALS(fminf(0.0f, 10000.0f), 0.0f);
		TS_ASSERT_EQUALS(fminf(100.0f, 10000.0f), 100.0f);
		TS_ASSERT_EQUALS(fminf(-1.0f, 2.0f), -1.0f);
		TS_ASSERT_EQUALS(fminf(-2.0f, 1.0f), -2.0f);
		TS_ASSERT_EQUALS(fminf(0.001f, 0.00001f), 0.00001f);

		TS_ASSERT_EQUALS(fmaxf(0.0f, 10000.0f), 10000.0f);
		TS_ASSERT_EQUALS(fmaxf(100.0f, 10000.0f), 10000.0f);
		TS_ASSERT_EQUALS(fmaxf(-1.0f, 2.0f), 2.0f);
		TS_ASSERT_EQUALS(fmaxf(-2.0f, 1.0f), 1.0f);
		TS_ASSERT_EQUALS(fmaxf(0.001f, 0.00001f), 0.001f);
	}

	void test_random()
	{
		u64 a = 0, b = 0;
		TS_ASSERT_OK(sys_generate_random_bytes((u8*)&a, sizeof(a)));
		TS_ASSERT_OK(sys_generate_random_bytes((u8*)&b, sizeof(b)));
		TS_ASSERT_DIFFERS(a, b);
	}

	void test_sys_get_executable_name()
	{
		fs::wpath path;

		// Try it first with the real executable (i.e. the
		// one that's running this test code)
		TS_ASSERT_EQUALS(sys_get_executable_name(path), INFO::OK);
		// Check it's absolute
		TSM_ASSERT(std::wstring(L"Path: ")+path.string(), path_is_absolute(path.string().c_str()));
		// Check the file exists
		struct stat s;
		TSM_ASSERT_EQUALS(std::wstring(L"Path: ")+path.string(), wstat(path.string().c_str(), &s), 0);

		// Do some platform-specific tests, based on the
		// implementations of sys_get_executable_name:

#if OS_LINUX
		// Since the implementation uses realpath, the tested files need to
		// really exist. So set up a directory tree for testing:

		const char* tmpdir = getenv("TMPDIR");
		if (! tmpdir) tmpdir = P_tmpdir;

		char root[PATH_MAX];
		sprintf_s(root, ARRAY_SIZE(root), "%s/pyrogenesis-test-sysdep-XXXXXX", tmpdir);
		TS_ASSERT(mkdtemp(root));
		std::string rootstr(root);
		std::wstring rootstrw(wstring_from_utf8(rootstr));

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
			FILE* f;
			errno_t err = fopen_s(&f, name.c_str(), "w");
			TS_ASSERT_EQUALS(err, 0);
			fclose(f);
		}

		// Try with absolute paths
		{
			Mock_dladdr d(rootstr+"/example/executable");
			TS_ASSERT_EQUALS(sys_get_executable_name(path), INFO::OK);
			TS_ASSERT_WSTR_EQUALS(path.string(), rootstrw+L"/example/executable");
		}
		{
			Mock_dladdr d(rootstr+"/example/./a/b/../e/../../executable");
			TS_ASSERT_EQUALS(sys_get_executable_name(path), INFO::OK);
			TS_ASSERT_WSTR_EQUALS(path.string(), rootstrw+L"/example/executable");
		}

		// Try with relative paths
		{
			Mock_dladdr d("./executable");
			Mock_getcwd m(rootstr+"/example");
			TS_ASSERT_EQUALS(sys_get_executable_name(path), INFO::OK);
			TS_ASSERT_WSTR_EQUALS(path.string(), rootstrw+L"/example/executable");
		}
		{
			Mock_dladdr d("./executable");
			Mock_getcwd m(rootstr+"/example/");
			TS_ASSERT_EQUALS(sys_get_executable_name(path), INFO::OK);
			TS_ASSERT_WSTR_EQUALS(path.string(), rootstrw+L"/example/executable");
		}
		{
			Mock_dladdr d("../d/../../f/executable");
			Mock_getcwd m(rootstr+"/example/a/b/c");
			TS_ASSERT_EQUALS(sys_get_executable_name(path), INFO::OK);
			TS_ASSERT_WSTR_EQUALS(path.string(), rootstrw+L"/example/a/f/executable");
		}

		// Try with pathless names
		{
			Mock_dladdr d("executable");
			TS_ASSERT_EQUALS(sys_get_executable_name(path), ERR::NO_SYS);
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
#endif // OS_LINUX
	}

	// Mock classes for test_sys_get_executable_name
#if OS_LINUX
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
#endif

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
