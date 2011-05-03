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

/*
 * helpers for built-in self tests
 */

/*

[KEEP IN SYNC WITH WIKI!]

Introduction
------------

Self-tests as advocated by eXtreme Programming have proven to be useful.
By embedding test code into modules, we can be confident that boundary
cases are handled correctly and everything still works after edits.
We give guidelines for their use and explain several helper mechanisms below.


Guidelines
----------

What makes a good self-test?
- They belong in the module being tested to ensure they are kept in
  sync with it.
- It is easiest to attach them to low-level functions, e.g. ceil_log2,
  rather than verifying the module's final result (e.g. checking renderer
  output by comparing pictures).
- You should cover all cases: expected failures ("does it fail as expected?"),
  bad inputs ("does it reject those?"), and successes ("did it have the
  expected result?").
- Tests should be non-intrusive (only bother user if something fails) and
  very quick. This is because they are executed every program run - which
  is a good thing because it solves the common problem of forgetting to
  run them after a change.

  If the test is unavoidably slow or annoying (example: wdbg_sym's
  stack trace), then best to disable it by default; see below for how.
  It can then be enabled manually after changes, and that is better than
  no test at all.


Example Usage
-------------

The following is a working example of a built-in self test using
our facilities. Further notes below are referenced with (1) etc.

>>>

#if SELF_TEST_ENABLED							// (1)
namespace test {								// (2)

static void test_log2()
{
	TEST(ilog2(0) == -1);						// (3)
	// further test cases..
}

static void self_test()
{
	test_log2();
	// further test groups..
}

SELF_TEST_RUN;									// (4)

}	// namespace test
#endif	// #if SELF_TEST_ENABLED

<<<

(1) when not enabled, self-tests are completely removed so as
    not to bloat the executable. for details on how to enable/disable them
	globally or override in single files, see below.

(2) wrapping in a namespace is optional and must be removed for C programs.
    it avoids possible name collisions with the module being tested.

(3) TEST *must* be used instead of ENSURE et al.! this is
    explained below.

(4) automatically calls your self_test function at non-local static object
    init time (i.e. before main is entered).

For further details, see below.

*/

#ifndef INCLUDED_SELF_TEST
#define INCLUDED_SELF_TEST

/*

// a self test is enabled if at the point of its definition
// SELF_TEST_ENABLED evaluates to 1 (non-zero).
// the value #defined below is the global default. you can override it
// in individual files by defining to 0 or 1 before including this header.
#ifndef SELF_TEST_ENABLED
#define SELF_TEST_ENABLED 1
#endif

// each test case should use this (instead of assert et al.) to verify
// conditions.
// rationale: some code checks boundary conditions via assert. these are
// often triggered deliberately in self-tests to verify error behavior.
// we therefore squelch asserts while tests are active (see mechanism below),
// and this is the only error reporter guaranteed to work.
//
// note: could also stringize condition and display that, but it'd require
// macro magic (stringize+prepend L) and we already display file+line.
#define TEST(condition) STMT(\
	if(!(condition))\
		debug_DisplayError(L"Self-test failed");\
)


// your source file should contain a function: void self_test() that
// performs all tests or calls out to individual test functions.
// this macro calls it at static init time and takes care of setting
// self_test_active (see above).
//
// rationale: since compiler optimizations may mess with the dummy variable,
// best to put this in a macro so we won't have to change each occurrence.
#define SELF_TEST_RUN\
	static int dummy = self_test_run(self_test)

// calling at static init time may not always be desirable - some
// self-tests may require initialization beforehand. this mechanism allows
// registering self tests automatically, which are then all run when you
// call self_test_run_all.
#define SELF_TEST_REGISTER\
	static SelfTestRecord self_test_record = { self_test, 0 };\
	static int dummy = self_test_register(&self_test_record)

struct SelfTestRecord
{
	void(*func)();
	const SelfTestRecord* next;
};

// call all self-tests registered thus far. rationale: see above.
// also displays a banner+elapsed time via debug_printf.
extern void self_test_run_all();


//
// internal use only:
//

// trampoline that sets self_test_active and returns a dummy value;
// used by SELF_TEST_RUN.
extern int self_test_run(void(*func)());

extern int self_test_register(SelfTestRecord* r);

// checked by debug_OnAssertionFailure; disables asserts if true (see above).
// set/cleared by run_self_test.
extern bool self_test_active;

*/


// for convenience, to avoid having to include all of these manually
#include "lib/status.h"
#include "lib/os_path.h"
#include "lib/posix/posix.h"

#define CXXTEST_HAVE_EH
#define CXXTEST_HAVE_STD

// If HAVE_STD wasn't defined at the point the ValueTraits header was included
// this header won't have been included and the default traits will be used for
// all variables... So fix that now ;-)
#include <cxxtest/StdValueTraits.h>
#include <cxxtest/TestSuite.h>

// Perform nice printing of CStr, based on std::string
#include "ps/CStr.h"
namespace CxxTest
{
	CXXTEST_TEMPLATE_INSTANTIATION
	class ValueTraits<const CStr8> : public ValueTraits<const CXXTEST_STD(string)>
	{
	public:
		ValueTraits( const CStr8 &s ) : ValueTraits<const CXXTEST_STD(string)>( s.c_str() ) {}
	};

	CXXTEST_COPY_CONST_TRAITS( CStr8 );

	CXXTEST_TEMPLATE_INSTANTIATION
	class ValueTraits<const CStrW> : public ValueTraits<const CXXTEST_STD(wstring)>
	{
	public:
		ValueTraits( const CStrW &s ) : ValueTraits<const CXXTEST_STD(wstring)>( s.c_str() ) {}
	};

	CXXTEST_COPY_CONST_TRAITS( CStrW );
}

// Perform nice printing of vectors
#include "maths/FixedVector3D.h"
#include "maths/Vector3D.h"
namespace CxxTest
{
	template<>
	class ValueTraits<CFixedVector3D>
	{
		CFixedVector3D v;
		std::string str;
	public:
		ValueTraits(const CFixedVector3D& v) : v(v)
		{
			std::stringstream s;
			s << "[" << v.X.ToDouble() << ", " << v.Y.ToDouble() << ", " << v.Z.ToDouble() << "]";
			str = s.str();
		}
		const char* asString() const
		{
			return str.c_str();
		}
	};

	template<>
	class ValueTraits<CVector3D>
	{
		CVector3D v;
		std::string str;
	public:
		ValueTraits(const CVector3D& v) : v(v)
		{
			std::stringstream s;
			s << "[" << v.X << ", " << v.Y << ", " << v.Z << "]";
			str = s.str();
		}
		const char* asString() const
		{
			return str.c_str();
		}
	};
}

#define TS_ASSERT_OK(expr) TS_ASSERT_EQUALS((expr), INFO::OK)
#define TSM_ASSERT_OK(m, expr) TSM_ASSERT_EQUALS(m, (expr), INFO::OK)
#define TS_ASSERT_STR_EQUALS(str1, str2) TS_ASSERT_EQUALS(std::string(str1), std::string(str2))
#define TSM_ASSERT_STR_EQUALS(m, str1, str2) TSM_ASSERT_EQUALS(m, std::string(str1), std::string(str2))
#define TS_ASSERT_WSTR_EQUALS(str1, str2) TS_ASSERT_EQUALS(std::wstring(str1), std::wstring(str2))
#define TSM_ASSERT_WSTR_EQUALS(m, str1, str2) TSM_ASSERT_EQUALS(m, std::wstring(str1), std::wstring(str2))
#define TS_ASSERT_PATH_EQUALS(path1, path2) TS_ASSERT_EQUALS((path1).string(), (path2).string())
#define TSM_ASSERT_PATH_EQUALS(m, path1, path2) TSM_ASSERT_EQUALS(m, (path1).string(), (path2).string())

bool ts_str_contains(const std::wstring& str1, const std::wstring& str2); // defined in test_setup.cpp
#define TS_ASSERT_WSTR_CONTAINS(str1, str2) TSM_ASSERT(str1, ts_str_contains(str1, str2))
#define TS_ASSERT_WSTR_NOT_CONTAINS(str1, str2) TSM_ASSERT(str1, !ts_str_contains(str1, str2))

template <typename T>
std::vector<T> ts_make_vector(T* start, size_t size_bytes)
{
	return std::vector<T>(start, start+(size_bytes/sizeof(T)));
}
#define TS_ASSERT_VECTOR_EQUALS_ARRAY(vec1, array) TS_ASSERT_EQUALS(vec1, ts_make_vector((array), sizeof(array)))

class ScriptInterface;
// Script-based testing setup (defined in test_setup.cpp). Defines TS_* functions.
void ScriptTestSetup(ScriptInterface&);

// Default game data directory
// (TODO: game-specific functions like this probably shouldn't be inside lib/, but it's useful
// here since lots of tests use it)
OsPath DataDir(); // defined in test_setup.cpp

#endif	// #ifndef INCLUDED_SELF_TEST
