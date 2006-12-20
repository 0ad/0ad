#include "lib/self_test.h"

#include "lib/lib.h"
#include "lib/self_test.h"
#include "lib/res/file/path.h"
#include "lib/res/file/file.h"

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

	void test_atom()
	{
		path_init();

		// file_make_unique_fn_copy

		// .. return same address for same string?
		const char* atom1 = file_make_unique_fn_copy("a/bc/def");
		const char* atom2 = file_make_unique_fn_copy("a/bc/def");
		TS_ASSERT_EQUALS(atom1, atom2);

		// .. early out (already in pool) check works?
		const char* atom3 = file_make_unique_fn_copy(atom1);
		TS_ASSERT_EQUALS(atom3, atom1);


		// path_is_atom_fn
		// is it reported as in pool?
		TS_ASSERT(path_is_atom_fn(atom1));

		// file_get_random_name
		// see if the atom added above eventually comes out when a
		// random one is returned from the pool.
		int tries_left;
		for(tries_left = 1000; tries_left != 0; tries_left--)
		{
			const char* random_name = file_get_random_name();
			if(random_name == atom1)
				break;
		}
		TS_ASSERT(tries_left != 0);

		path_shutdown();
	}
};
