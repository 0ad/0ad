/* Copyright (C) 2017 Wildfire Games.
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
#include "maths/FixedVector2D.h"
#include "maths/FixedVector3D.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"

#include "jsapi.h"

class TestScriptConversions : public CxxTest::TestSuite
{
	template <typename T>
	void convert_to(const T& value, const std::string& expected)
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		TS_ASSERT(script.LoadGlobalScripts());
		JSContext* cx = script.GetContext();
		JSAutoRequest rq(cx);

		JS::RootedValue v1(cx);
		ScriptInterface::ToJSVal(cx, &v1, value);

		// We want to convert values to strings, but can't just call toSource() on them
		// since they might not be objects. So just use uneval.
		std::string source;
		JS::RootedValue global(cx, script.GetGlobalObject());
		TS_ASSERT(script.CallFunction(global, "uneval", source, v1));

		TS_ASSERT_STR_EQUALS(source, expected);
	}

	template <typename T>
	void roundtrip(const T& value, const char* expected)
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		TS_ASSERT(script.LoadGlobalScripts());
		JSContext* cx = script.GetContext();
		JSAutoRequest rq(cx);

		JS::RootedValue v1(cx);
		ScriptInterface::ToJSVal(cx, &v1, value);

		std::string source;
		JS::RootedValue global(cx, script.GetGlobalObject());
		TS_ASSERT(script.CallFunction(global, "uneval", source, v1));

		if (expected)
			TS_ASSERT_STR_EQUALS(source, expected);

		T v2 = T();
		TS_ASSERT(ScriptInterface::FromJSVal(cx, v1, v2));
		TS_ASSERT_EQUALS(value, v2);
	}

	template <typename T>
	void call_prototype_function(const T& u, const T& v, const std::string& func, const std::string& expected)
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		TS_ASSERT(script.LoadGlobalScripts());
		JSContext* cx = script.GetContext();
		JSAutoRequest rq(cx);

		JS::RootedValue v1(cx);
		ScriptInterface::ToJSVal(cx, &v1, v);
		JS::RootedValue u1(cx);
		ScriptInterface::ToJSVal(cx, &u1, u);

		T r;
		JS::RootedValue r1(cx);

		TS_ASSERT(script.CallFunction(u1, func.c_str(), r, v1));
		ScriptInterface::ToJSVal(cx, &r1, r);

		std::string source;
		JS::RootedValue global(cx, script.GetGlobalObject());
		TS_ASSERT(script.CallFunction(global, "uneval", source, r1));

		TS_ASSERT_STR_EQUALS(source, expected);
	}

public:
	void setUp()
	{
		g_VFS = CreateVfs();
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir()/"mods"/"_test.sim", VFS_MOUNT_MUST_EXIST));
	}

	void tearDown()
	{
		g_VFS.reset();
	}

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
		roundtrip<i32>(-1073741823, "-1073741823"); // JSVAL_INT_MIN+1
		roundtrip<i32>(-1073741824, "-1073741824"); // JSVAL_INT_MIN

		roundtrip<u32>(0, "0");
		roundtrip<u32>(123, "123");
		roundtrip<u32>(1073741822, "1073741822"); // JSVAL_INT_MAX-1
		roundtrip<u32>(1073741823, "1073741823"); // JSVAL_INT_MAX

		{
			TestLogger log; // swallow warnings about values not being stored as integer JS::Values
			roundtrip<i32>(1073741824, "1073741824"); // JSVAL_INT_MAX+1
			roundtrip<i32>(-1073741825, "-1073741825"); // JSVAL_INT_MIN-1
			roundtrip<u32>(1073741824, "1073741824"); // JSVAL_INT_MAX+1
		}

		std::string s1 = "test";
		s1[1] = '\0';
		std::string s2 = "тест";
		s2[2] = s2[3] = '\0';

		std::wstring w1 = L"test";
		w1[1] = '\0';
		std::wstring w2 = L"тест";
		w2[1] = '\0';

		roundtrip<std::string>("", "\"\"");
		roundtrip<std::string>("test", "\"test\"");
		roundtrip<std::string>("тест", "\"\\xD1\\x82\\xD0\\xB5\\xD1\\x81\\xD1\\x82\"");
		roundtrip<std::string>(s1, "\"t\\x00st\"");
		roundtrip<std::string>(s2, "\"\\xD1\\x82\\x00\\x00\\xD1\\x81\\xD1\\x82\"");

		roundtrip<std::wstring>(L"", "\"\"");
		roundtrip<std::wstring>(L"test", "\"test\"");
		// Windows has two byte wchar_t. We test for this explicitly since we can catch more possible issues this way.
		roundtrip<std::wstring>(L"тест", sizeof(wchar_t) == 2 ? "\"\\xD1\\u201A\\xD0\\xB5\\xD1\\x81\\xD1\\u201A\"" : "\"\\u0442\\u0435\\u0441\\u0442\"");
		roundtrip<std::wstring>(w1, "\"t\\x00st\"");
		roundtrip<std::wstring>(w2, sizeof(wchar_t) == 2 ? "\"\\xD1\\x00\\xD0\\xB5\\xD1\\x81\\xD1\\u201A\"" : "\"\\u0442\\x00\\u0441\\u0442\"");

		convert_to<const char*>("", "\"\"");
		convert_to<const char*>("test", "\"test\"");
		convert_to<const char*>(s1.c_str(), "\"t\"");
		convert_to<const char*>(s2.c_str(), "\"\\xD1\\x82\"");

		roundtrip<fixed>(fixed::FromInt(0), "0");
		roundtrip<fixed>(fixed::FromInt(123), "123");
		roundtrip<fixed>(fixed::FromInt(-123), "-123");
		roundtrip<fixed>(fixed::FromDouble(123.25), "123.25");
	}

	void test_integers()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		JSContext* cx = script.GetContext();
		JSAutoRequest rq(cx);

		// using new uninitialized variables each time to be sure the test doesn't succeeed if ToJSVal doesn't touch the value at all.
		JS::RootedValue val0(cx), val1(cx), val2(cx), val3(cx), val4(cx), val5(cx), val6(cx), val7(cx), val8(cx);
		ScriptInterface::ToJSVal<i32>(cx, &val0, 0);
		ScriptInterface::ToJSVal<i32>(cx, &val1, 2147483646); // JSVAL_INT_MAX-1
		ScriptInterface::ToJSVal<i32>(cx, &val2, 2147483647); // JSVAL_INT_MAX
		ScriptInterface::ToJSVal<i32>(cx, &val3, -2147483647); // JSVAL_INT_MIN+1
		ScriptInterface::ToJSVal<i32>(cx, &val4, -(i64)2147483648u); // JSVAL_INT_MIN
		TS_ASSERT(val0.isInt32());
		TS_ASSERT(val1.isInt32());
		TS_ASSERT(val2.isInt32());
		TS_ASSERT(val3.isInt32());
		TS_ASSERT(val4.isInt32());

		ScriptInterface::ToJSVal<u32>(cx, &val5, 0);
		ScriptInterface::ToJSVal<u32>(cx, &val6, 2147483646u); // JSVAL_INT_MAX-1
		ScriptInterface::ToJSVal<u32>(cx, &val7, 2147483647u); // JSVAL_INT_MAX
		ScriptInterface::ToJSVal<u32>(cx, &val8, 2147483648u); // JSVAL_INT_MAX+1
		TS_ASSERT(val5.isInt32());
		TS_ASSERT(val6.isInt32());
		TS_ASSERT(val7.isInt32());
		TS_ASSERT(val8.isDouble());
	}

	void test_nonfinite()
	{
		roundtrip<float>(std::numeric_limits<float>::infinity(), "Infinity");
		roundtrip<float>(-std::numeric_limits<float>::infinity(), "-Infinity");
		convert_to<float>(std::numeric_limits<float>::quiet_NaN(), "NaN"); // can't use roundtrip since nan != nan

		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		JSContext* cx = script.GetContext();
		JSAutoRequest rq(cx);

		float f = 0;
		JS::RootedValue testNANVal(cx);
		ScriptInterface::ToJSVal(cx, &testNANVal, NAN);
		TS_ASSERT(ScriptInterface::FromJSVal(cx, testNANVal, f));
		TS_ASSERT(isnan(f));
	}

	// NOTE: fixed and vector conversions are defined in simulation2/scripting/EngineScriptConversions.cpp

	void test_fixed()
	{
		fixed f;

		f.SetInternalValue(10590283);
		roundtrip<fixed>(f, "161.5948944091797");

		f.SetInternalValue(-10590283);
		roundtrip<fixed>(f, "-161.5948944091797");

		f.SetInternalValue(2000000000);
		roundtrip<fixed>(f, "30517.578125");

		f.SetInternalValue(2000000001);
		roundtrip<fixed>(f, "30517.57814025879");
	}

	void test_vector2d()
	{
		CFixedVector2D v(fixed::Zero(), fixed::Pi());
		roundtrip<CFixedVector2D>(v, "({x:0, y:3.1415863037109375})");

		CFixedVector2D u(fixed::FromInt(1), fixed::Zero());
		call_prototype_function<CFixedVector2D>(u, v, "add", "({x:1, y:3.1415863037109375})");
	}

	void test_vector3d()
	{
		CFixedVector3D v(fixed::Zero(), fixed::Pi(), fixed::FromInt(1));
		roundtrip<CFixedVector3D>(v, "({x:0, y:3.1415863037109375, z:1})");

		CFixedVector3D u(fixed::Pi(), fixed::Zero(), fixed::FromInt(2));
		call_prototype_function<CFixedVector3D>(u, v, "add", "({x:3.1415863037109375, y:3.1415863037109375, z:3})");
	}
};
