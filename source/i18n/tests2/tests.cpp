/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "Interface.h"
#include "scripting/SpiderMonkey.h"

#define FILE_PATH "..\\data\\mods\\official\\language\\test\\"

extern I18n::CLocale_interface* g_CurrentLocale;
#define translate(x) g_CurrentLocale->Translate(x)

extern std::string readfile(const char* fn);

#include <stdarg.h>

typedef char CHAR;
typedef const CHAR *LPCSTR;

extern "C" {
__declspec(dllimport) void __stdcall
OutputDebugStringA(LPCSTR lpOutputString);
}
/*
WINBASEAPI
VOID
WINAPI
OutputDebugStringA(
	IN LPCSTR lpOutputString
);
*/

void test_assert(bool test, const char* file, int line, const char* fmt, ...)
{
	if (! test)
	{
		char msg[512];
		va_list v;
		va_start(v, fmt);
		_vsnprintf(msg, sizeof(msg), fmt, v);
		char err[512];
		_snprintf(err, sizeof(err), "%s(%d) : TEST FAILED : %s\n", file, line, msg);
		err[sizeof(err)-1] = 0;
		OutputDebugStringA(err);
		printf("%s", err);
	}
}

#ifndef I18NDEBUG
#error Please compile with I18NDEBUG debug
#endif
namespace I18n { extern bool g_UsedCache; }

#define TEST_EQ_NOCACHE(answer, str, param) \
{ \
	I18n::Str s = translate(L##str) param; \
	test_assert(s == L##answer, __FILE__, __LINE__, "Unexpected output: %ls", s.c_str()); \
	test_assert(!I18n::g_UsedCache, __FILE__, __LINE__, "Unexpectedly used cache"); \
}

#define TEST_EQ_CACHED(answer, str, param) \
{ \
	I18n::Str s = translate(L##str) param; \
	test_assert(s == L##answer, __FILE__, __LINE__, "Unexpected output: %ls", s.c_str()); \
	test_assert(I18n::g_UsedCache, __FILE__, __LINE__, "Failed to use cache"); \
}

#define TEST_EQ_SCRIPT(answer, str) \
{ \
	jsval rval; \
	if (JS_EvaluateScript(cx, glob, str, (int)strlen(str), "test", 1, &rval) != JS_TRUE) \
		test_assert(false, __FILE__, __LINE__, "Script eval failed"); \
	else { \
		JSString*	 r = JS_ValueToString(cx, rval); \
		I18n::Str s (JS_GetStringChars(r), JS_GetStringLength(r)); \
		test_assert(s == L##answer, __FILE__, __LINE__, "Unexpected output:\n '%ls'\nExpected:\n '%ls'", s.c_str(), L##answer); \
	} \
}

void test(JSContext* cx, JSObject* glob)
{
	// Untranslated strings. Simple strings.
	TEST_EQ_NOCACHE(
		"Hello world",
		"Hello world",
		);

	// Reporting of an invalid number of parameters.
	TEST_EQ_NOCACHE(
		"(translation error)",
		"Hello world", << 1
		);

	// Untranslated strings with variables. Int variables.
	TEST_EQ_NOCACHE(
		"Hello world 1",
		"Hello world $n", << 1
		);

	// Parsing of untranslated strings. Int variables.
	TEST_EQ_NOCACHE(
		"Hello world 1 23$4!",
		"Hello world $a $bee$cee$$$bee!", << 1 << 2 << 3 << 4
		);
	// PROBLEM: Duplicated variables aren't handled correctly (or at all).
	// Make sure they're never used in the identifier strings.

	// Caching with ints.
	TEST_EQ_CACHED(
		"Hello world 1",
		"Hello world $n", << 1
		);

	// Double variables.
	TEST_EQ_NOCACHE(
		"Hello world 1.500000",
		"Hello world $n", << 1.5
		);

	// Caching with doubles.
	TEST_EQ_CACHED(
		"Hello world 1.500000",
		"Hello world $n", << 1.5
		);

/*
	// More caching with doubles.
	int num[2] = { -1, 0x3ff00000 }; // slightly endian-dependent
	TEST_EQ_NOCACHE(
		"Hello world 1.000001",
		"Hello world $n", << *(double*)&num;
		);
	
	num[1] = 0xbff00000;
	TEST_EQ_NOCACHE(
		"Hello world -1.000001",
		"Hello world $n", << *(double*)&num;
	);

	num[0] = -2;
	TEST_EQ_NOCACHE(
		"Hello world -1.000001",
		"Hello world $n", << *(double*)&num;
	);
*/

	// Float variables.
	TEST_EQ_NOCACHE(
		"Hello world 2.500000",
		"Hello world $n", << 2.5f
		);

	// Caching with floats.
	TEST_EQ_CACHED(
		"Hello world 2.500000",
		"Hello world $n", << 2.5f
		);

	// Handling nouns (no noun table loaded).
	TEST_EQ_NOCACHE(
		"Hello world cheese",
		"Hello world $n", << I18n::Noun("cheese")
		);

	g_CurrentLocale->LoadDictionary(readfile(FILE_PATH "nouns2.wrd").c_str());

	// Handling nouns (not in noun table).
	TEST_EQ_NOCACHE(
		"Hello world pumpkin",
		"Hello world $n", << I18n::Noun("pumpkin")
		);

	// Handling nouns (from noun table but missing singular).
	TEST_EQ_NOCACHE(
		"Hello world apple",
		"Hello world $n", << I18n::Noun("apple")
		);

	g_CurrentLocale->UnloadDictionaries();

	g_CurrentLocale->LoadDictionary(readfile(FILE_PATH "nouns.wrd").c_str());

	// Name variables.
	TEST_EQ_NOCACHE(
		"Hello world banana",
		"Hello world $n", << I18n::Name("banana")
		);

	// Raw string variables. Caching names / raw strings.
	TEST_EQ_CACHED(
		"Hello world banana",
		"Hello world $n", << I18n::Raw("banana")
		);

	// Handling nouns (from noun table).
	TEST_EQ_NOCACHE(
		"Hello world BANANA",
		"Hello world $n", << I18n::Noun("banana")
		);

	// Caching nouns.
	TEST_EQ_CACHED(
		"Hello world BANANA",
		"Hello world $n", << I18n::Noun("banana")
		);

	std::string funcs = readfile(FILE_PATH "functions.js");
	g_CurrentLocale->LoadFunctions(funcs.c_str(), funcs.size(), "functions.txt");

	g_CurrentLocale->LoadStrings(readfile(FILE_PATH "phrases.lng").c_str());

	// Functions with strings, doubles, ints, variable ints, and variable strings.
	TEST_EQ_NOCACHE(
		"1.2345+67890=67891.2345. An armadillo buys a platypus for 14.99 (platypus! 14.990000!)",
		"Test $obj ($$$amnt)", << I18n::Noun("platypus") << 14.99
		);

	// Noun lookups in JS functions.
	TEST_EQ_NOCACHE(
		"There are (1 BANANA) here",
		"Testing things $num of $object", << 1 << I18n::Noun("banana")
		);

	// More noun lookups in JS functions.
	TEST_EQ_NOCACHE(
		"There are (2 bananas) here",
		"Testing things $num of $object", << 2 << I18n::Noun("banana")
		);

	// Noun lookups in JS functions, despite being Raw. (JS only sees strings).
	TEST_EQ_NOCACHE(
		"There are (2.5 bananas) here",
		"Testing things $num of $object", << 2.5 << I18n::Raw("banana")
		);

	// Noun lookup failures in JS functions.
	TEST_EQ_NOCACHE(
		"There are (-3.5 orangutan) here",
		"Testing things $num of $object", << -3.5 << I18n::Raw("orangutan")
		);

	// Repeated tests, using JS interface.

	TEST_EQ_SCRIPT(
		"Hello world banana",
		"translate('Hello world $n', i18n.Name('banana'))"
		);

	TEST_EQ_SCRIPT(
		"Hello world banana",
		"translate('Hello world $n', i18n.Raw('banana'))"
		);

	TEST_EQ_SCRIPT(
		"Hello world BANANA",
		"translate('Hello world $n', i18n.Noun('banana'))"
		);

	TEST_EQ_SCRIPT(
		"1.2345+67890=67891.2345. An armadillo buys a platypus for 14.99 (platypus! 14.990000!)",
		"translate('Test $obj ($$$amnt)', i18n.Noun('platypus'), 14.99)"
		);

	TEST_EQ_SCRIPT(
		"There are (1 BANANA) here",
		"translate('Testing things $num of $object', 1, i18n.Noun('banana'))"
		);

	TEST_EQ_SCRIPT(
		"There are (2 bananas) here",
		"translate('Testing things $num of $object', 2, i18n.Noun('banana'))"
		);

	TEST_EQ_SCRIPT(
		"There are (2.5 bananas) here",
		"translate('Testing things $num of $object', 2.5, i18n.Raw('banana'))"
		);

	TEST_EQ_SCRIPT(
		"There are (-3.5 orangutan) here",
		"translate('Testing things $num of $object', -3.5, i18n.Raw('orangutan'))"
		);
}