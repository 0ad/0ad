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

#include <stdio.h>
#include <assert.h>
#include <time.h>

#include "Interface.h"

#include "scripting/SpiderMonkey.h"

I18n::CLocale_interface* g_CurrentLocale;
#define translate(x) g_CurrentLocale->Translate(x)

std::string readfile(const char* fn)
{
	std::string t;
	FILE* f = fopen(fn, "rb");
	assert(f);
	size_t c;
	char buf[1024];
	while (0 != (c = fread(buf, 1, sizeof(buf), f)))
		t += std::string(buf, buf+c);
	fclose(f);
	return t;
}

void errrep(JSContext* cx, const char* msg, JSErrorReport* err)
{
	printf("Error: %s\n", msg);
}

int main()
{
	JSRuntime* rt = JS_NewRuntime(1024*1024);
	assert(rt);
	JSContext* cx = JS_NewContext(rt, 8192);
	assert(cx);

	JSClass clas =
	{
		"global", 0,
		JS_PropertyStub, JS_PropertyStub,
		JS_PropertyStub, JS_PropertyStub,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub
	};

	JSObject* glob = JS_NewObject(cx, &clas, NULL, NULL);
	JSBool builtins = JS_InitStandardClasses(cx, glob);
	assert(builtins);

	JS_SetErrorReporter(cx, errrep);

#ifndef NDEBUG
//	_CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF);
	//CrtSetBreakAlloc(1450);
#endif

	g_CurrentLocale = I18n::NewLocale(cx, glob);
	if (!g_CurrentLocale)
		return 1;


	extern void test(JSContext*, JSObject*);
	test(cx, glob);

#if 0
	std::string lang = readfile("e:\\0ad\\cvs\\binaries\\data\\mods\\official\\language\\test\\phrases.lng");
	std::string funcs = readfile("e:\\0ad\\cvs\\binaries\\data\\mods\\official\\language\\test\\functions.js");
	std::string words = readfile("e:\\0ad\\cvs\\binaries\\data\\mods\\official\\language\\test\\nouns.wrd");
	std::string words2 = readfile("e:\\0ad\\cvs\\binaries\\data\\mods\\official\\language\\test\\nouns2.wrd");

	g_CurrentLocale->LoadFunctions(funcs.c_str(), funcs.size(), "functions.txt");
	g_CurrentLocale->LoadStrings(lang.c_str());
	g_CurrentLocale->LoadDictionary(words.c_str());
	g_CurrentLocale->LoadDictionary(words2.c_str());

	I18n::Str s;

//	const char* script = " translate('Hello $name!', i18n.Noun('apple')) ";
	const char* script = " translate('Testing things $num of $object', 3/1.5, i18n.Noun('banana')) ";

	jsval rval;
	if (JS_EvaluateScript(cx, glob, script, (int)strlen(script), "test", 1, &rval) != JS_TRUE)
		assert(! "Eval failed");
	//assert(JSVAL_IS_STRING(rval));
	//s = JS_GetStringChars(JSVAL_TO_STRING(rval));
	s = JS_GetStringChars(JS_ValueToString(cx, rval));
	printf("%ls\n", s.c_str());

#if 0
	s = translate(L"Hello $name!") << I18n::Name("banana");
	printf("%ls\n", s.c_str());
	s = translate(L"Hello $name!") << I18n::Name("banana");
	printf("%ls\n", s.c_str());

	s = translate(L"Testing things $num of $object") << 1 << I18n::Name("banana");
	printf("%ls\n", s.c_str());
	s = translate(L"Testing things $num of $object") << 1234 << I18n::Name("banana");
	printf("%ls\n", s.c_str());
	s = translate(L"Testing things $num of $object") << 12345 << I18n::Name("apple");
	printf("%ls\n", s.c_str());
	s = translate(L"Testing things $num of $object") << 123456 << I18n::Name("orange");
	printf("%ls\n", s.c_str());
	s = translate(L"Testing things $num of $object") << 1234567 << I18n::Name("cheese");
	printf("%ls\n", s.c_str());

	s = translate(L"Hello $name!") << I18n::Name("Philip");
	printf("%ls\n", s.c_str());

	s = translate(L"Hello $name!") << I18n::Name("Philip2");
	printf("%ls\n", s.c_str());

	//	s = translate(L"Also hi to $you$me etc") << I18n::DateShort(time(NULL)) << -1;
	//	printf("%ls\n", s.c_str());

	//	s = translate(L"Hello $name!") << I18n::DateShort(time(NULL));
	//char* y = "you";
	//s = translate(L"Hello $name!") << y;
	//	printf("%ls\n", s.c_str());

	s = translate(L"Your $colour cheese-monster eats $num biscuits") << I18n::Name("blue") << 15;
	printf("%ls\n", s.c_str());
#endif

// Performance test:  (it should go at a few hundred thousand per second)
#if 0
	clock_t t = clock();
	int limit = 1000000;
	for (int i=0; i<limit; ++i)
	{
		/*
		I18n::Str s1 = translate(L"Hello world 1");
		I18n::Str s2 = translate(L"Hello world 2");
		I18n::Str s3 = translate(L"Hello world 3");
		I18n::Str s4 = translate(L"Hello world 4");
		//		I18n::Str s1 = translate(L"Hello $name!") << L"Philip";
		//		I18n::Str s2 = translate(L"Your $colour cheese-monster eats $num biscuits") << "blue" << 15;
		//I18n::Str s3 = translate(L"Hello2 $name!") << L"Philip";
		//I18n::Str s4 = translate(L"Your2 $colour cheese-monster eats $num biscuits") << "blue" << 15;
		//		I18n::Str s3 = translate(L"Hello $name!") << 1234;
		//		I18n::Str s4 = translate(L"Your $colour cheese-monster eats $num biscuits") << 10000 << 15;
		/*/

		I18n::Str s1 = translate(L"Testing things $num $obj") << 1234.0 << I18n::Name("cheese bananas");
		//I18n::Str s2 = translate(L"Testing things $num") << 124;
		//I18n::Str s3 = translate(L"Testing things $num") << 1231234;
		//I18n::Str s4 = translate(L"Testing things $num") << 1243456;

		//*/
	}
	t = clock() - t;
	printf("\nTook %f secs (%f/sec)\n", (float)t/(float)CLOCKS_PER_SEC, (float)limit/((float)t/(float)CLOCKS_PER_SEC));
#endif
#endif

	delete g_CurrentLocale;

	JS_DestroyContext(cx);
	JS_DestroyRuntime(rt);
	JS_ShutDown();

	getchar();
}