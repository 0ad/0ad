#include "lib/self_test.h"

#include "lib/self_test.h"
#include "lib/res/file/path.h"

class TestPath : public CxxTest::TestSuite 
{
public:
	void test_conversion()
	{
		char N_path[PATH_MAX] = {0};
		TS_ASSERT_OK(file_make_native_path("a/b/c", N_path));
#if OS_WIN
		TS_ASSERT_STR_EQUALS(N_path, "a\\b\\c");
#else
		TS_ASSERT_STR_EQUALS(N_path, "a/b/c");
#endif

		char P_path[PATH_MAX] = {0};
		TS_ASSERT_OK(file_make_portable_path("a\\b\\c", P_path));
#if OS_WIN
		TS_ASSERT_STR_EQUALS(P_path, "a/b/c");
#else
		// sounds strange, but correct: on non-Windows, \\ didn't
		// get recognized as separators and weren't converted.
		TS_ASSERT_STR_EQUALS(P_path, "a\\b\\c");
#endif

	}

	// file_make_full_*_path is left untested (hard to do so)

	void test_pool()
	{
		// .. return same address for same string?
		const char* atom1 = path_Pool->UniqueCopy("a/bc/def");
		const char* atom2 = path_Pool->UniqueCopy("a/bc/def");
		TS_ASSERT_EQUALS(atom1, atom2);

		// .. early out (already in pool) check works?
		const char* atom3 = path_Pool->UniqueCopy(atom1);
		TS_ASSERT_EQUALS(atom3, atom1);

		// is it reported as in pool?
		TS_ASSERT(path_Pool()->Contains(atom1));

		// path_Pool()->RandomString
		// see if the atom added above eventually comes out when a
		// random one is returned from the pool.
		int tries_left;
		for(tries_left = 1000; tries_left != 0; tries_left--)
		{
			const char* random_name = path_Pool->RandomString();
			if(random_name == atom1)
				break;
		}
		TS_ASSERT(tries_left != 0);
	}
};
