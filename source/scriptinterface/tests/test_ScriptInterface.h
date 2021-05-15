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
#include "scriptinterface/JSON.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/StructuredClone.h"

#include "ps/CLogger.h"

#include <boost/random/linear_congruential.hpp>

class TestScriptInterface : public CxxTest::TestSuite
{
public:
	void test_loadscript_basic()
	{
		ScriptInterface script("Test", "Test", g_ScriptContext);
		TestLogger logger;
		TS_ASSERT(script.LoadScript(L"test.js", "var x = 1+1;"));
		TS_ASSERT_STR_NOT_CONTAINS(logger.GetOutput(), "JavaScript error");
		TS_ASSERT_STR_NOT_CONTAINS(logger.GetOutput(), "JavaScript warning");
	}

	void test_loadscript_error()
	{
		ScriptInterface script("Test", "Test", g_ScriptContext);
		TestLogger logger;
		TS_ASSERT(!script.LoadScript(L"test.js", "1+"));
		TS_ASSERT_STR_CONTAINS(logger.GetOutput(), "ERROR: JavaScript error: test.js line 2\nexpected expression, got \'}\'");
	}

	void test_loadscript_strict_warning()
	{
		ScriptInterface script("Test", "Test", g_ScriptContext);
		TestLogger logger;
		// in strict mode, this inside a function doesn't point to the global object
		TS_ASSERT(script.LoadScript(L"test.js", "var isStrict = (function() { return !this; })();warn('isStrict is '+isStrict);"));
		TS_ASSERT_STR_CONTAINS(logger.GetOutput(), "WARNING: isStrict is true");
	}

	void test_loadscript_strict_error()
	{
		ScriptInterface script("Test", "Test", g_ScriptContext);
		TestLogger logger;
		TS_ASSERT(!script.LoadScript(L"test.js", "with(1){}"));
		TS_ASSERT_STR_CONTAINS(logger.GetOutput(), "ERROR: JavaScript error: test.js line 1\nstrict mode code may not contain \'with\' statements");
	}

	void test_clone_basic()
	{
		ScriptInterface script1("Test", "Test", g_ScriptContext);
		ScriptInterface script2("Test", "Test", g_ScriptContext);

		ScriptRequest rq1(script1);
		JS::RootedValue obj1(rq1.cx);
		TS_ASSERT(script1.Eval("({'x': 123, 'y': [1, 1.5, '2', 'test', undefined, null, true, false]})", &obj1));

		{
			ScriptRequest rq2(script2);

			JS::RootedValue obj2(rq2.cx, Script::CloneValueFromOtherCompartment(script2, script1, obj1));

			std::string source;
			TS_ASSERT(ScriptFunction::Call(rq2, obj2, "toSource", source));
			TS_ASSERT_STR_EQUALS(source, "({x:123, y:[1, 1.5, \"2\", \"test\", (void 0), null, true, false]})");
		}
	}

	void test_clone_getters()
	{
		// The tests should be run with JS_SetGCZeal so this can try to find GC bugs
		ScriptInterface script1("Test", "Test", g_ScriptContext);
		ScriptInterface script2("Test", "Test", g_ScriptContext);

		ScriptRequest rq1(script1);

		JS::RootedValue obj1(rq1.cx);
		TS_ASSERT(script1.Eval("var s = '?'; var v = ({get x() { return 123 }, 'y': {'w':{get z() { delete v.y; delete v.n; v = null; s += s; return 4 }}}, 'n': 100}); v", &obj1));

		{
			ScriptRequest rq2(script2);

			JS::RootedValue obj2(rq2.cx, Script::CloneValueFromOtherCompartment(script2, script1, obj1));

			std::string source;
			TS_ASSERT(ScriptFunction::Call(rq2, obj2, "toSource", source));
			TS_ASSERT_STR_EQUALS(source, "({x:123, y:{w:{z:4}}})");
		}
	}

	void test_clone_cyclic()
	{
		ScriptInterface script1("Test", "Test", g_ScriptContext);
		ScriptInterface script2("Test", "Test", g_ScriptContext);

		ScriptRequest rq1(script1);

		JS::RootedValue obj1(rq1.cx);
		TS_ASSERT(script1.Eval("var x = []; x[0] = x; ({'a': x, 'b': x})", &obj1));

		{
			ScriptRequest rq2(script2);
			JS::RootedValue obj2(rq2.cx, Script::CloneValueFromOtherCompartment(script2, script1, obj1));

			// Use JSAPI function to check if the values of the properties "a", "b" are equals a.x[0]
			JS::RootedValue prop_a(rq2.cx);
			JS::RootedValue prop_b(rq2.cx);
			JS::RootedValue prop_x1(rq2.cx);
			TS_ASSERT(Script::GetProperty(rq2, obj2, "a", &prop_a));
			TS_ASSERT(Script::GetProperty(rq2, obj2, "b", &prop_b));
			TS_ASSERT(prop_a.isObject());
			TS_ASSERT(prop_b.isObject());
			TS_ASSERT(Script::GetProperty(rq2, prop_a, "0", &prop_x1));
			TS_ASSERT(prop_x1.get() == prop_a.get());
			TS_ASSERT(prop_x1.get() == prop_b.get());
		}
	}

	/**
	 * This test is mainly to make sure that all required template overloads get instantiated at least once so that compiler errors
	 * in these functions are revealed instantly (but it also tests the basic functionality of these functions).
	 */
	void test_rooted_templates()
	{
		ScriptInterface script("Test", "Test", g_ScriptContext);

		ScriptRequest rq(script);

		JS::RootedValue val(rq.cx);
		JS::RootedValue out(rq.cx);
		TS_ASSERT(script.Eval("({ "
			"'0':0,"
			"inc:function() { this[0]++; return this[0]; }, "
			"setTo:function(nbr) { this[0] = nbr; }, "
			"add:function(nbr) { this[0] += nbr; return this[0]; } "
			"})"
			, &val));

		JS::RootedValue nbrVal(rq.cx, JS::NumberValue(3));
		int nbr = 0;

		ScriptFunction::CallVoid(rq, val, "setTo", nbrVal);

		// Test that a mutable handle value as return value works.
		ScriptFunction::Call(rq, val, "inc", &out);

		Script::FromJSVal(rq, out, nbr);
		TS_ASSERT_EQUALS(4, nbr);

		ScriptFunction::Call(rq, val, "add", nbr, nbrVal);
		TS_ASSERT_EQUALS(7, nbr);

		// GetProperty JS::RootedValue* overload
		nbr = 0;
		Script::GetProperty(rq, val, "0", &out);
		Script::FromJSVal(rq, out, nbr);
		TS_ASSERT_EQUALS(nbr, 7);

		// GetPropertyInt JS::RootedValue* overload
		nbr = 0;
		Script::GetPropertyInt(rq, val, 0, &out);
		Script::FromJSVal(rq, out, nbr);
		TS_ASSERT_EQUALS(nbr, 7);

		handle_templates_test(script, val, &out, nbrVal);
	}

	void handle_templates_test(const ScriptInterface& script, JS::HandleValue val, JS::MutableHandleValue out, JS::HandleValue nbrVal)
	{
		ScriptRequest rq(script);

		int nbr = 0;

		ScriptFunction::CallVoid(rq, val, "setTo", nbrVal);
		ScriptFunction::Call(rq, val, "inc", out);

		Script::FromJSVal(rq, out, nbr);
		TS_ASSERT_EQUALS(4, nbr);

		ScriptFunction::Call(rq, val, "add", nbr, nbrVal);
		TS_ASSERT_EQUALS(7, nbr);

		// GetProperty JS::MutableHandleValue overload
		nbr = 0;
		Script::GetProperty(rq, val, "0", out);
		Script::FromJSVal(rq, out, nbr);
		TS_ASSERT_EQUALS(nbr, 7);

		// GetPropertyInt JS::MutableHandleValue overload
		nbr = 0;
		Script::GetPropertyInt(rq, val, 0, out);
		Script::FromJSVal(rq, out, nbr);
		TS_ASSERT_EQUALS(nbr, 7);
	}

	void test_random()
	{
		ScriptInterface script("Test", "Test", g_ScriptContext);

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
		ScriptInterface script("Test", "Test", g_ScriptContext);
		ScriptRequest rq(script);

		std::string input = "({'x':1,'z':[2,'3\\u263A\\ud800'],\"y\":true})";
		JS::RootedValue val(rq.cx);
		TS_ASSERT(script.Eval(input.c_str(), &val));

		std::string stringified = Script::StringifyJSON(rq, &val);
		TS_ASSERT_STR_EQUALS(stringified, "{\n  \"x\": 1,\n  \"z\": [\n    2,\n    \"3\u263A\\ud800\"\n  ],\n  \"y\": true\n}");

		TS_ASSERT(Script::ParseJSON(rq, stringified, &val));
		TS_ASSERT_STR_EQUALS(Script::ToString(rq, &val), "({x:1, z:[2, \"3\\u263A\\uD800\"], y:true})");
	}

	// This function tests a common way to mod functions, by creating a wrapper that
	// extends the functionality and is then assigned to the name of the function.
	void test_function_override()
	{
		ScriptInterface script("Test", "Test", g_ScriptContext);
		ScriptRequest rq(script);

		TS_ASSERT(script.Eval(
			"function f() { return 1; }"
			"f = (function (originalFunction) {"
				"return function () { return originalFunction() + 1; }"
			"})(f);"
		));

		JS::RootedValue out(rq.cx);
		TS_ASSERT(script.Eval("f()", &out));

		int outNbr = 0;
		Script::FromJSVal(rq, out, outNbr);
		TS_ASSERT_EQUALS(2, outNbr);
	}
};
