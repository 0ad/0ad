/* Copyright (C) 2015 Wildfire Games.
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

#ifndef INCLUDED_SELF_TEST
#define INCLUDED_SELF_TEST

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

// TODO: The CStr and Vector code should not be in lib/
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

bool ts_str_contains(const std::string& str1, const std::string& str2); // defined in test_setup.cpp
bool ts_str_contains(const std::wstring& str1, const std::wstring& str2); // defined in test_setup.cpp
#define TS_ASSERT_STR_CONTAINS(str1, str2) TSM_ASSERT(str1, ts_str_contains(str1, str2))
#define TS_ASSERT_STR_NOT_CONTAINS(str1, str2) TSM_ASSERT(str1, !ts_str_contains(str1, str2))
#define TS_ASSERT_WSTR_CONTAINS(str1, str2) TSM_ASSERT(str1, ts_str_contains(str1, str2))
#define TS_ASSERT_WSTR_NOT_CONTAINS(str1, str2) TSM_ASSERT(str1, !ts_str_contains(str1, str2))

template <typename T>
std::vector<T> ts_make_vector(T* start, size_t size_bytes)
{
	return std::vector<T>(start, start+(size_bytes/sizeof(T)));
}
#define TS_ASSERT_VECTOR_EQUALS_ARRAY(vec1, array) TS_ASSERT_EQUALS(vec1, ts_make_vector((array), sizeof(array)))
#define TS_ASSERT_VECTOR_CONTAINS(vec1, element) TS_ASSERT(std::find((vec1).begin(), (vec1).end(), element) != (vec1).end());

class ScriptInterface;
// Script-based testing setup (defined in test_setup.cpp). Defines TS_* functions.
void ScriptTestSetup(ScriptInterface&);

// Default game data directory
// (TODO: game-specific functions like this probably shouldn't be inside lib/, but it's useful
// here since lots of tests use it)
OsPath DataDir(); // defined in test_setup.cpp

#endif	// #ifndef INCLUDED_SELF_TEST
