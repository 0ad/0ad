#include "lib/lib.h"
#include "lib/self_test.h"

#include "lib/path_util.h"

class TestPathUtil : public CxxTest::TestSuite 
{
	void TEST_APPEND(const char* path1, const char* path2, uint flags, const char* correct_result)
	{
		char dst[PATH_MAX] = {0};
		TS_ASSERT_OK(path_append(dst, path1, path2, flags));
		TS_ASSERT_STR_EQUALS(dst, correct_result);
	}

	// if correct_ret is ERR_FAIL, ignore correct_result.
	void TEST_REPLACE(const char* src, const char* remove, const char* replace,
		LibError correct_ret, const char* correct_result)
	{
		char dst[PATH_MAX] = {0};
		TS_ASSERT_EQUALS(path_replace(dst, src, remove, replace), correct_ret);
		if(correct_ret != ERR_FAIL)
			TS_ASSERT_STR_EQUALS(dst, correct_result);
	}

	void TEST_NAME_ONLY(const char* path, const char* correct_result)
	{
		const char* result = path_name_only(path);
		TS_ASSERT_STR_EQUALS(result, correct_result);
	}

	void TEST_LAST_COMPONENT(const char* path, const char* correct_result)
	{
		const char* result = path_last_component(path);
		TS_ASSERT_STR_EQUALS(result, correct_result);
	}

	void TEST_STRIP_FN(const char* path_readonly, const char* correct_result)
	{
		char path[PATH_MAX];
		path_copy(path, path_readonly);
		path_strip_fn(path);
		TS_ASSERT_STR_EQUALS(path, correct_result);
	}

	void TEST_PATH_EXT(const char* path, const char* correct_result)
	{
		const char* result = path_extension(path);
		TS_ASSERT_STR_EQUALS(result, correct_result);
	}

	void TEST_PATH_PACKAGE(const char* path, const char* fn,
		const char* correct_result)
	{
		PathPackage pp;
		TS_ASSERT_OK(path_package_set_dir(&pp, path));
		TS_ASSERT_OK(path_package_append_file(&pp, fn));
		TS_ASSERT_STR_EQUALS(pp.path, correct_result);
	}


public:

	void test_subpath()
	{
		// obvious true
		TS_ASSERT(path_is_subpath("abc/def/", "abc/def/") == true);	// same
		TS_ASSERT(path_is_subpath("abc/def/", "abc/") == true);	// 2 is subpath
		TS_ASSERT(path_is_subpath("abc/", "abc/def/") == true);	// 1 is subpath

		// nonobvious true
		TS_ASSERT(path_is_subpath("", "") == true);
		TS_ASSERT(path_is_subpath("abc/def/", "abc/def") == true);	// no '/' !

		// obvious false
		TS_ASSERT(path_is_subpath("abc", "def") == false);	// different, no path

		// nonobvious false
		TS_ASSERT(path_is_subpath("abc", "") == false);	// empty comparand
		// .. different but followed by common subdir
		TS_ASSERT(path_is_subpath("abc/def/", "ghi/def/") == false);
		TS_ASSERT(path_is_subpath("abc/def/", "abc/ghi") == false);
	}

	// TODO: can't test path validate yet without suppress-error-dialog

	void test_append()
	{
		// simplest case
		TEST_APPEND("abc", "def", 0, "abc/def");
		// trailing slash
		TEST_APPEND("abc", "def", PATH_APPEND_SLASH, "abc/def/");
		// intervening slash
		TEST_APPEND("abc/", "def", 0, "abc/def");
		// nonportable intervening slash
		TEST_APPEND("abc\\", "def", 0, "abc\\def");
		// mixed path slashes
		TEST_APPEND("abc", "def/ghi\\jkl", 0, "abc/def/ghi\\jkl");
		// path1 empty
		TEST_APPEND("", "abc/def/", 0, "abc/def/");
		// path2 empty, no trailing slash
		TEST_APPEND("abc/def", "", 0, "abc/def");
		// path2 empty, require trailing slash
		TEST_APPEND("abc/def", "", PATH_APPEND_SLASH, "abc/def/");
		// require trailing slash, already exists
		TEST_APPEND("abc/", "def/", PATH_APPEND_SLASH, "abc/def/");
	}

	void test_replace()
	{
		// no match
		TEST_REPLACE("abc/def", "/def", "xx", ERR_FAIL, 0);
		// normal case: match and remove
		TEST_REPLACE("abc/def", "abc", "ok", INFO_OK, "ok/def");
		// caller also stripping /
		TEST_REPLACE("abc/def", "abc/", "ok", INFO_OK, "ok/def");
		// empty remove
		TEST_REPLACE("abc/def", "", "ok", INFO_OK, "ok/abc/def");
		// empty replace
		TEST_REPLACE("abc/def", "abc", "", INFO_OK, "def");
		// remove entire string
		TEST_REPLACE("abc/def", "abc/def", "", INFO_OK, "");
	}

	void test_name_only()
	{
		// path with filename
		TEST_NAME_ONLY("abc/def", "def");
		// nonportable path with filename
		TEST_NAME_ONLY("abc\\def\\ghi", "ghi");
		// mixed path with filename
		TEST_NAME_ONLY("abc/def\\ghi", "ghi");
		// mixed path with filename (2)
		TEST_NAME_ONLY("abc\\def/ghi", "ghi");
		// filename only
		TEST_NAME_ONLY("abc", "abc");
		// empty
		TEST_NAME_ONLY("", "");
	}

	void test_last_component()
	{
		// path with filename
		TEST_LAST_COMPONENT("abc/def", "def");
		// nonportable path with filename
		TEST_LAST_COMPONENT("abc\\def\\ghi", "ghi");
		// mixed path with filename
		TEST_LAST_COMPONENT("abc/def\\ghi", "ghi");
		// mixed path with filename (2)
		TEST_LAST_COMPONENT("abc\\def/ghi", "ghi");
		// filename only
		TEST_LAST_COMPONENT("abc", "abc");
		// empty
		TEST_LAST_COMPONENT("", "");

		// now paths (mostly copied from above test series)

		// path
		TEST_LAST_COMPONENT("abc/def/", "def/");
		// nonportable path
		TEST_LAST_COMPONENT("abc\\def\\ghi\\", "ghi\\");
		// mixed path
		TEST_LAST_COMPONENT("abc/def\\ghi/", "ghi/");
		// mixed path (2)
		TEST_LAST_COMPONENT("abc\\def/ghi\\", "ghi\\");
	}

	void test_strip_fn()
	{
		// path with filename
		TEST_STRIP_FN("abc/def", "abc/");
		// nonportable path with filename
		TEST_STRIP_FN("abc\\def\\ghi", "abc\\def\\");
		// mixed path with filename
		TEST_STRIP_FN("abc/def\\ghi", "abc/def\\");
		// mixed path with filename (2)
		TEST_STRIP_FN("abc\\def/ghi", "abc\\def/");
		// filename only
		TEST_STRIP_FN("abc", "");
		// empty
		TEST_STRIP_FN("", "");
		// path
		TEST_STRIP_FN("abc/def/", "abc/def/");
		// nonportable path
		TEST_STRIP_FN("abc\\def\\ghi\\", "abc\\def\\ghi\\");
	}

	// note: no need to test path_dir_only - it is implemented exactly as
	// done in TEST_STRIP_FN.

	void test_path_ext()
	{
		TEST_PATH_EXT("a/b/c.bmp", "bmp");
		TEST_PATH_EXT("a.BmP", "BmP");	// case sensitive
		TEST_PATH_EXT("c", "");	// no extension
		TEST_PATH_EXT("", "");	// empty
	}

	// testing path_foreach_component is difficult; currently skipped.

	void test_path_package()
	{
		// normal
		TEST_PATH_PACKAGE("a/b", "c", "a/b/c");
		// nonportable slash
		TEST_PATH_PACKAGE("a\\b", "c", "a\\b/c");
		// slash already present
		TEST_PATH_PACKAGE("a/b/", "c", "a/b/c");
		// nonportable slash already present
		TEST_PATH_PACKAGE("a\\b\\", "c", "a\\b\\c");
		// mixed slashes
		TEST_PATH_PACKAGE("a/b\\c", "d", "a/b\\c/d");
		// mixed slashes (2)
		TEST_PATH_PACKAGE("a\\b/c", "d", "a\\b/c/d");
	}
};
