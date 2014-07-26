/* Copyright (C) 2012 Wildfire Games.
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

#include "ps/CLogger.h"

#include <boost/random/linear_congruential.hpp>

class TestScriptInterface : public CxxTest::TestSuite
{
public:
	void test_loadscript_basic()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		TestLogger logger;
		TS_ASSERT(script.LoadScript(L"test.js", "var x = 1+1;"));
		TS_ASSERT_WSTR_NOT_CONTAINS(logger.GetOutput(), L"JavaScript error");
		TS_ASSERT_WSTR_NOT_CONTAINS(logger.GetOutput(), L"JavaScript warning");
	}

	void test_loadscript_error()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		TestLogger logger;
		TS_ASSERT(!script.LoadScript(L"test.js", "1+"));
		TS_ASSERT_WSTR_CONTAINS(logger.GetOutput(), L"JavaScript error: test.js line 1\nSyntaxError: syntax error");
	}

	void test_loadscript_strict_warning()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		TestLogger logger;
		// in strict mode, this inside a function doesn't point to the global object
		TS_ASSERT(script.LoadScript(L"test.js", "var isStrict = (function() { return !this; })();warn('isStrict is '+isStrict);"));
		TS_ASSERT_WSTR_CONTAINS(logger.GetOutput(), L"WARNING: isStrict is true");
	}

	void test_loadscript_strict_error()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		TestLogger logger;
		TS_ASSERT(!script.LoadScript(L"test.js", "with(1){}"));
		TS_ASSERT_WSTR_CONTAINS(logger.GetOutput(), L"JavaScript error: test.js line 1\nSyntaxError: strict mode code may not contain \'with\' statements");
	}

	void test_clone_basic()
	{
		ScriptInterface script1("Test", "Test", g_ScriptRuntime);
		ScriptInterface script2("Test", "Test", g_ScriptRuntime);

		CScriptVal obj1;
		TS_ASSERT(script1.Eval("({'x': 123, 'y': [1, 1.5, '2', 'test', undefined, null, true, false]})", obj1));

		CScriptVal obj2 = script2.CloneValueFromOtherContext(script1, obj1.get());

		std::string source;
		TS_ASSERT(script2.CallFunction(obj2.get(), "toSource", source));
		TS_ASSERT_STR_EQUALS(source, "({x:123, y:[1, 1.5, \"2\", \"test\", (void 0), null, true, false]})");
	}

	void test_clone_getters()
	{
		// The tests should be run with JS_SetGCZeal so this can try to find GC bugs
		ScriptInterface script1("Test", "Test", g_ScriptRuntime);
		ScriptInterface script2("Test", "Test", g_ScriptRuntime);

		CScriptVal obj1;
		TS_ASSERT(script1.Eval("var s = '?'; var v = ({get x() { return 123 }, 'y': {'w':{get z() { delete v.y; delete v.n; v = null; s += s; return 4 }}}, 'n': 100}); v", obj1));

		CScriptVal obj2 = script2.CloneValueFromOtherContext(script1, obj1.get());

		std::string source;
		TS_ASSERT(script2.CallFunction(obj2.get(), "toSource", source));
		TS_ASSERT_STR_EQUALS(source, "({x:123, y:{w:{z:4}}})");
	}

	void test_clone_cyclic()
	{
		ScriptInterface script1("Test", "Test", g_ScriptRuntime);
		ScriptInterface script2("Test", "Test", g_ScriptRuntime);

		CScriptVal obj1;
		TS_ASSERT(script1.Eval("var x = []; x[0] = x; ({'a': x, 'b': x})", obj1));

		CScriptVal obj2 = script2.CloneValueFromOtherContext(script1, obj1.get());

		// Use JSAPI function to check if the values of the properties "a", "b" are equals a.x[0]
		JSContext* cx2 = script2.GetContext();
		JSAutoRequest rq(cx2);
		JS::RootedValue prop_a(cx2);
		JS::RootedValue prop_b(cx2);
		JS::RootedValue prop_x1(cx2);
		TS_ASSERT(JS_GetProperty(cx2, &(obj2.get().toObject()), "a", prop_a.address()));
		TS_ASSERT(JS_GetProperty(cx2, &(obj2.get().toObject()), "b", prop_b.address()));
		TS_ASSERT(prop_a.get().isObject());
		TS_ASSERT(prop_b.get().isObject());
		TS_ASSERT(JS_GetProperty(cx2, &(prop_a.get().toObject()), "0", prop_x1.address()));
		TS_ASSERT_EQUALS(prop_x1.get(), prop_a.get());
		TS_ASSERT_EQUALS(prop_x1.get(), prop_b.get());
	}
	
	/**
	 * This test is mainly to make sure that all required template overloads get instantiated at least once so that compiler errors
	 * in these functions are revealed instantly (but it also tests the basic functionality of these functions).
	 */
	void test_rooted_templates()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);

		JSContext* cx = script.GetContext();
		JSAutoRequest rq(cx);
		
		JS::RootedValue val(cx);
		JS::RootedValue out(cx);
		TS_ASSERT(script.Eval("({ '0':0, inc:function() { this[0]++; return this[0]; }, setTo:function(nbr) { this[0] = nbr; } })", &val));
	
		JS::RootedValue nbrVal(cx, JS::NumberValue(3));
		int nbr = 0;
		
		// CallFunctionVoid JS::RootedValue& overload
		script.CallFunctionVoid(val, "setTo", nbrVal);

		// CallFunction JS::RootedValue* overload
		script.CallFunction(val, "inc", &out);
		
		ScriptInterface::FromJSVal(cx, out, nbr);
		TS_ASSERT_EQUALS(4, nbr);

		// GetProperty tests JS::RootedValue* overload
		nbr = 0;
		script.GetProperty(val, "0", &out); // JS::RootedValue* overload
		ScriptInterface::FromJSVal(cx, out, nbr);
		TS_ASSERT_EQUALS(nbr, 4);

		// GetPropertyInt tests JS::RootedValue* overload
		nbr = 0;
		script.GetPropertyInt(val, 0, &out);
		ScriptInterface::FromJSVal(cx, out, nbr);
		TS_ASSERT_EQUALS(nbr, 4);

		handle_templates_test(script, val, &out, nbrVal);
	}

	void handle_templates_test(ScriptInterface& script, JS::HandleValue val, JS::MutableHandleValue out, JS::HandleValue nbrVal)
	{
		int nbr = 0;

		// CallFunctionVoid JS::HandleValue overload
		script.CallFunctionVoid(val, "setTo", nbrVal);

		// CallFunction JS::MutableHandleValue overload
		script.CallFunction(val, "inc", out);
		
		ScriptInterface::FromJSVal(script.GetContext(), out, nbr);
		TS_ASSERT_EQUALS(4, nbr);

		// GetProperty JS::MutableHandleValue overload
		nbr = 0;
		script.GetProperty(val, "0", out);
		ScriptInterface::FromJSVal(script.GetContext(), out, nbr);
		TS_ASSERT_EQUALS(nbr, 4);

		// GetPropertyInt JS::MutableHandleValue overload
		nbr = 0;
		script.GetPropertyInt(val, 0, out);
		ScriptInterface::FromJSVal(script.GetContext(), out, nbr);
		TS_ASSERT_EQUALS(nbr, 4);
	}

	void test_random()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);

		double d1, d2;
		TS_ASSERT(script.Eval("Math.random()", d1));
		TS_ASSERT(script.Eval("Math.random()", d2));
		TS_ASSERT_DIFFERS(d1, d2);

		boost::rand48 rng;
		script.ReplaceNondeterministicRNG(rng);
		rng.seed((u64)0);
		TS_ASSERT(script.Eval("Math.random()", d1));
		TS_ASSERT(script.Eval("Math.random()", d2));
		TS_ASSERT_DIFFERS(d1, d2);
		rng.seed((u64)0);
		TS_ASSERT(script.Eval("Math.random()", d2));
		TS_ASSERT_EQUALS(d1, d2);
	}

	void test_json()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);

		std::string input = "({'x':1,'z':[2,'3\\u263A\\ud800'],\"y\":true})";
		CScriptValRooted val;
		TS_ASSERT(script.Eval(input.c_str(), val));

		std::string stringified = script.StringifyJSON(val.get());
		TS_ASSERT_STR_EQUALS(stringified, "{\n  \"x\": 1,\n  \"z\": [\n    2,\n    \"3\xE2\x98\xBA\xEF\xBF\xBD\"\n  ],\n  \"y\": true\n}");

		val = script.ParseJSON(stringified);
		TS_ASSERT_WSTR_EQUALS(script.ToString(val.get()), L"({x:1, z:[2, \"3\\u263A\\uFFFD\"], y:true})");
	}
};
