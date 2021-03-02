/* Copyright (C) 2021 Wildfire Games.
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

#include "scriptinterface/FunctionWrapper.h"

class TestFunctionWrapper : public CxxTest::TestSuite
{
public:

	// TODO C++20: use lambda functions directly, names are 'N params, void/returns'.
	static void _1p_v(int) {};
	static void _3p_v(int, bool, std::string) {};
	static int _3p_r(int a, bool, std::string) { return a; };

	static void _0p_v() {};
	static int _0p_r() { return 1; };

	void test_simple_wrappers()
	{
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::_1p_v>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::_3p_v>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::_3p_r>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::_0p_v>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::_0p_r>), JSNative>);
	}

	static void _handle(JS::HandleValue) {};
	static void _handle_2(int, JS::HandleValue, bool) {};

	static void _cmpt_private(ScriptInterface::CmptPrivate*) {};
	static int _cmpt_private_2(ScriptInterface::CmptPrivate*, int a, bool) { return a; };

	static void _script_request(const ScriptRequest&) {};
	static int _script_request_2(const ScriptRequest&, int a, bool) { return a; };

	void test_special_wrappers()
	{
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::_handle>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::_handle_2>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::_cmpt_private>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::_cmpt_private_2>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::_script_request>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::_script_request_2>), JSNative>);
	}

	class test_method
	{
	public:
		void method_1() {};
		int method_2(int, const int&) { return 4; };
		void const_method_1() const {};
		int const_method_2(int, const int&) const { return 4; };
	};

	void test_method_wrappers()
	{
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::test_method::method_1,
											  &ScriptFunction::ObjectFromCBData<test_method>>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::test_method::method_2,
											  &ScriptFunction::ObjectFromCBData<test_method>>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::test_method::const_method_1,
											  &ScriptFunction::ObjectFromCBData<test_method>>), JSNative>);
		static_assert(std::is_same_v<decltype(&ScriptFunction::ToJSNative<&TestFunctionWrapper::test_method::const_method_2,
											  &ScriptFunction::ObjectFromCBData<test_method>>), JSNative>);
	}

	void test_calling()
	{
		ScriptInterface script("Test", "Test", g_ScriptContext);
		ScriptRequest rq(script);

		ScriptFunction::Register<&TestFunctionWrapper::_1p_v>(script, "_1p_v");
		{
			std::string input = "Test._1p_v(0);";
			JS::RootedValue val(rq.cx);
			TS_ASSERT(script.Eval(input.c_str(), &val));
		}

		ScriptFunction::Register<&TestFunctionWrapper::_3p_r>(script, "_3p_r");
		{
			std::string input = "Test._3p_r(4, false, 'test');";
			int ret = 0;
			TS_ASSERT(script.Eval(input.c_str(), ret));
			TS_ASSERT_EQUALS(ret, 4);
		}

		ScriptFunction::Register<&TestFunctionWrapper::_cmpt_private_2>(script, "_cmpt_private_2");
		{
			std::string input = "Test._cmpt_private_2(4);";
			int ret = 0;
			TS_ASSERT(script.Eval(input.c_str(), ret));
			TS_ASSERT_EQUALS(ret, 4);
		}
	}
};
