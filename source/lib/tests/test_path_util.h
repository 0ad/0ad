#include "lib/lib.h"
#include "lib/self_test.h"

#include "lib/path_util.h"

// Macros, not functions, to get proper line number reports when tests fail
#define TEST_APPEND(path1, path2, flags, correct_result) \
	{ \
		char dst[PATH_MAX] = {0}; \
		path_append(dst, path1, path2, flags); \
		TS_ASSERT_STR_EQUALS(dst, correct_result); \
	}

#define TEST_NAME_ONLY(path, correct_result) \
{ \
	const char* result = path_name_only(path); \
	TS_ASSERT_STR_EQUALS(result, correct_result); \
}

#define TEST_STRIP_FN(path_readonly, correct_result) \
{ \
	char path[PATH_MAX]; \
	path_copy(path, path_readonly); \
	path_strip_fn(path); \
	TS_ASSERT_STR_EQUALS(path, correct_result); \
}

class TestPathUtil : public CxxTest::TestSuite 
{
	void TEST_PATH_EXT(const char* path, const char* correct_result)
	{
		const char* result = path_extension(path);
		TS_ASSERT_STR_EQUALS(result, correct_result);
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

	void test_path_ext()
	{
		TEST_PATH_EXT("a/b/c.bmp", "bmp");
		TEST_PATH_EXT("a.BmP", "BmP");	// case sensitive
		TEST_PATH_EXT("c", "");	// no extension
		TEST_PATH_EXT("", "");	// empty
	}
};
