/* Copyright (C) 2010 Wildfire Games.
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

#include "lib/self_test.h"

#include "scriptinterface/ScriptInterface.h"

#include "maths/Fixed.h"
#include "maths/MathUtil.h"

#include "js/jsapi.h"

class TestScriptConversions : public CxxTest::TestSuite
{
	template <typename T>
	void convert_to(const T& value, const std::string& expected)
	{
		ScriptInterface script("Test");
		JSContext* cx = script.GetContext();

		jsval v1 = ScriptInterface::ToJSVal(cx, value);
		JS_AddRoot(cx, &v1);

		// We want to convert values to strings, but can't just call toSource() on them
		// since they might not be objects. So just use uneval.
		std::string source;
		TS_ASSERT(script.CallFunction(OBJECT_TO_JSVAL(JS_GetGlobalObject(cx)), "uneval", CScriptVal(v1), source));

		TS_ASSERT_STR_EQUALS(source, expected);

		JS_RemoveRoot(cx, &v1);
	}

	template <typename T>
	void roundtrip(const T& value, const std::string& expected)
	{
		ScriptInterface script("Test");
		JSContext* cx = script.GetContext();

		jsval v1 = ScriptInterface::ToJSVal(cx, value);
		JS_AddRoot(cx, &v1);

		std::string source;
		TS_ASSERT(script.CallFunction(OBJECT_TO_JSVAL(JS_GetGlobalObject(cx)), "uneval", CScriptVal(v1), source));

		TS_ASSERT_STR_EQUALS(source, expected);

		T v2 = T();
		TS_ASSERT(ScriptInterface::FromJSVal(script.GetContext(), v1, v2));
		TS_ASSERT_EQUALS(value, v2);

		JS_RemoveRoot(cx, &v1);
	}

public:
	void test_roundtrip()
	{
		roundtrip<bool>(true, "true");
		roundtrip<bool>(false, "false");

		roundtrip<float>(0, "0");
		roundtrip<float>(0.5, "0.5");
		roundtrip<float>(1e9f, "1000000000");
		roundtrip<float>(1e30f, "1.0000000150474662e+30");

		roundtrip<i32>(0, "0");
		roundtrip<i32>(123, "123");
		roundtrip<i32>(-123, "-123");
		roundtrip<i32>(1073741822, "1073741822"); // JSVAL_INT_MAX-1
		roundtrip<i32>(1073741823, "1073741823"); // JSVAL_INT_MAX
		roundtrip<i32>(1073741824, "1073741824"); // JSVAL_INT_MAX+1
#if JS_VERSION >= 180
		roundtrip<i32>(-1073741823, "-1073741823"); // JSVAL_INT_MIN+1
		roundtrip<i32>(-1073741824, "-1073741824"); // JSVAL_INT_MIN
		roundtrip<i32>(-1073741825, "-1073741825"); // JSVAL_INT_MIN-1
#else
		roundtrip<i32>(-1073741822, "-1073741822"); // JSVAL_INT_MIN+1
		roundtrip<i32>(-1073741823, "-1073741823"); // JSVAL_INT_MIN
		roundtrip<i32>(-1073741824, "-1073741824"); // JSVAL_INT_MIN-1
#endif

		roundtrip<u32>(0, "0");
		roundtrip<u32>(123, "123");
		roundtrip<u32>(1073741822, "1073741822"); // JSVAL_INT_MAX-1
		roundtrip<u32>(1073741823, "1073741823"); // JSVAL_INT_MAX
		roundtrip<u32>(1073741824, "1073741824"); // JSVAL_INT_MAX+1

		std::string s1 = "test";
		s1[1] = '\0';

		std::wstring w1 = L"test";
		w1[1] = '\0';

		roundtrip<std::string>("", "\"\"");
		roundtrip<std::string>("test", "\"test\"");
#if JS_VERSION >= 180
		roundtrip<std::string>(s1, "\"t\\0st\"");
#else
		roundtrip<std::string>(s1, "\"t\\x00st\"");
#endif
		// TODO: should test non-ASCII strings

		roundtrip<std::wstring>(L"", "\"\"");
		roundtrip<std::wstring>(L"test", "\"test\"");
#if JS_VERSION >= 180
		roundtrip<std::wstring>(w1, "\"t\\0st\"");
#else
		roundtrip<std::wstring>(w1, "\"t\\x00st\"");
#endif
		// TODO: should test non-ASCII strings

		convert_to<const char*>("", "\"\"");
		convert_to<const char*>("test", "\"test\"");
		convert_to<const char*>(s1.c_str(), "\"t\"");

		roundtrip<CFixed_23_8>(CFixed_23_8::FromInt(0), "0");
		roundtrip<CFixed_23_8>(CFixed_23_8::FromInt(123), "123");
		roundtrip<CFixed_23_8>(CFixed_23_8::FromInt(-123), "-123");
		roundtrip<CFixed_23_8>(CFixed_23_8::FromDouble(123.25), "123.25");
	}

	void test_integers()
	{
		ScriptInterface script("Test");
		JSContext* cx = script.GetContext();

		TS_ASSERT(JSVAL_IS_INT(ScriptInterface::ToJSVal<i32>(cx, 0)));

		TS_ASSERT(JSVAL_IS_INT(ScriptInterface::ToJSVal<i32>(cx, 1073741822))); // JSVAL_INT_MAX-1
		TS_ASSERT(JSVAL_IS_INT(ScriptInterface::ToJSVal<i32>(cx, 1073741823))); // JSVAL_INT_MAX
		TS_ASSERT(JSVAL_IS_DOUBLE(ScriptInterface::ToJSVal<i32>(cx, 1073741824))); // JSVAL_INT_MAX+1
#if JS_VERSION >= 180
		TS_ASSERT(JSVAL_IS_INT(ScriptInterface::ToJSVal<i32>(cx, -1073741823))); // JSVAL_INT_MIN+1
		TS_ASSERT(JSVAL_IS_INT(ScriptInterface::ToJSVal<i32>(cx, -1073741824))); // JSVAL_INT_MIN
		TS_ASSERT(JSVAL_IS_DOUBLE(ScriptInterface::ToJSVal<i32>(cx, -1073741825))); // JSVAL_INT_MIN-1
#else
		TS_ASSERT(JSVAL_IS_INT(ScriptInterface::ToJSVal<i32>(cx, -1073741822))); // JSVAL_INT_MIN+1
		TS_ASSERT(JSVAL_IS_INT(ScriptInterface::ToJSVal<i32>(cx, -1073741823))); // JSVAL_INT_MIN
		TS_ASSERT(JSVAL_IS_DOUBLE(ScriptInterface::ToJSVal<i32>(cx, -1073741824))); // JSVAL_INT_MIN-1
#endif

		TS_ASSERT(JSVAL_IS_INT(ScriptInterface::ToJSVal<u32>(cx, 0)));

		TS_ASSERT(JSVAL_IS_INT(ScriptInterface::ToJSVal<u32>(cx, 1073741822))); // JSVAL_INT_MAX-1
		TS_ASSERT(JSVAL_IS_INT(ScriptInterface::ToJSVal<u32>(cx, 1073741823))); // JSVAL_INT_MAX
		TS_ASSERT(JSVAL_IS_DOUBLE(ScriptInterface::ToJSVal<u32>(cx, 1073741824))); // JSVAL_INT_MAX+1
	}

	void test_nonfinite()
	{
		roundtrip<float>(INFINITY, "Infinity");
		roundtrip<float>(-INFINITY, "-Infinity");
		convert_to<float>(NAN, "NaN"); // can't use roundtrip since nan != nan

		ScriptInterface script("Test");
		JSContext* cx = script.GetContext();

		float f = 0;
		TS_ASSERT(ScriptInterface::FromJSVal(cx, ScriptInterface::ToJSVal(cx, NAN), f));
		TS_ASSERT(isnan(f));
	}

	// TODO: test vectors
};
