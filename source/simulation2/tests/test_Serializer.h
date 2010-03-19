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

#include "simulation2/serialization/DebugSerializer.h"
#include "simulation2/serialization/HashSerializer.h"
#include "simulation2/serialization/StdSerializer.h"
#include "simulation2/serialization/StdDeserializer.h"
#include "scriptinterface/ScriptInterface.h"

#include "ps/CLogger.h"

#include <iostream>

#define TS_ASSERT_STREAM(stream, len, buffer) \
	TS_ASSERT_EQUALS(stream.str().length(), (size_t)len); \
	TS_ASSERT_SAME_DATA(stream.str().data(), buffer, len)
#define TSM_ASSERT_STREAM(m, stream, len, buffer) \
	TSM_ASSERT_EQUALS(m, stream.str().length(), (size_t)len); \
	TSM_ASSERT_SAME_DATA(m, stream.str().data(), buffer, len)

class TestSerializer : public CxxTest::TestSuite
{
public:
	void serialize_types(ISerializer& serialize)
	{
		serialize.NumberU8_Unbounded("u8", 255);
		serialize.NumberI32_Unbounded("i32", -123);
		serialize.NumberU32_Unbounded("u32", (unsigned)-123);
		serialize.NumberFloat_Unbounded("float", 1e+30f);
		serialize.NumberDouble_Unbounded("double", 1e+300);
		serialize.NumberFixed_Unbounded("fixed", CFixed_23_8::FromFloat(1234.5f));

		serialize.Bool("t", true);
		serialize.Bool("f", false);

		serialize.StringASCII("string", "example", 0, 255);
		serialize.StringASCII("string 2", "example\"\\\"", 0, 255);
		serialize.StringASCII("string 3", "example\n\ntest", 0, 255);

		wchar_t testw[] = { 't', 0xEA, 's', 't', 0 };
		serialize.String("string 4", testw, 0, 255);

		serialize.RawBytes("raw bytes", (const u8*)"\0\1\2\3\x0f\x10", 6);
	}

	void test_Debug_basic()
	{
		ScriptInterface script("Test");
		std::stringstream stream;
		CDebugSerializer serialize(script, stream);
		serialize.NumberI32_Unbounded("x", -123);
		serialize.NumberU32_Unbounded("y", 1234);
		serialize.NumberI32("z", 12345, 0, 65535);
		TS_ASSERT_STR_EQUALS(stream.str(), "x: -123\ny: 1234\nz: 12345\n");
	}

	void test_Debug_floats()
	{
		ScriptInterface script("Test");
		std::stringstream stream;
		CDebugSerializer serialize(script, stream);
		serialize.NumberFloat_Unbounded("x", 1e4f);
		serialize.NumberFloat_Unbounded("x", 1e-4f);
		serialize.NumberFloat_Unbounded("x", 1e5f);
		serialize.NumberFloat_Unbounded("x", 1e-5f);
		serialize.NumberFloat_Unbounded("x", 1e6f);
		serialize.NumberFloat_Unbounded("x", 1e-6f);
		serialize.NumberFloat_Unbounded("x", 1e10f);
		serialize.NumberFloat_Unbounded("x", 1e-10f);
		serialize.NumberDouble_Unbounded("x", 1e4);
		serialize.NumberDouble_Unbounded("x", 1e-4);
		serialize.NumberDouble_Unbounded("x", 1e5);
		serialize.NumberDouble_Unbounded("x", 1e-5);
		serialize.NumberDouble_Unbounded("x", 1e6);
		serialize.NumberDouble_Unbounded("x", 1e-6);
		serialize.NumberDouble_Unbounded("x", 1e10);
		serialize.NumberDouble_Unbounded("x", 1e-10);
		serialize.NumberDouble_Unbounded("x", 1e100);
		serialize.NumberDouble_Unbounded("x", 1e-100);
		serialize.NumberFixed_Unbounded("x", CFixed_23_8::FromDouble(1e6));
		TS_ASSERT_STR_EQUALS(stream.str(),
				"x: 10000\nx: 0.0001\nx: 100000\nx: 1e-05\nx: 1e+06\nx: 1e-06\nx: 1e+10\nx: 1e-10\n"
				"x: 10000\nx: 0.0001\nx: 100000\nx: 1e-05\nx: 1e+06\nx: 1e-06\nx: 1e+10\nx: 1e-10\nx: 1e+100\nx: 1e-100\n"
				"x: 1e+06\n"
		);
	}

	void test_Debug_types()
	{
		ScriptInterface script("Test");
		std::stringstream stream;
		CDebugSerializer serialize(script, stream);

		serialize.Comment("comment");

		serialize_types(serialize);

		TS_ASSERT_STR_EQUALS(stream.str(),
				"# comment\n"
				"u8: 255\n"
				"i32: -123\n"
				"u32: 4294967173\n"
				"float: 1e+30\n"
				"double: 1e+300\n"
				"fixed: 1234.5\n"
				"t: true\n"
				"f: false\n"
				"string: \"example\"\n"
				"string 2: \"example\\\"\\\\\\\"\"\n" // C-escaped form of: "example\"\\\""
				"string 3: \"example\\n\\ntest\"\n"
				"string 4: \"t\xC3\xAAst\"\n"
				"raw bytes: (6 bytes) 00 01 02 03 0f 10\n"
		);
	}

	void test_Std_basic()
	{
		ScriptInterface script("Test");
		std::stringstream stream;
		CStdSerializer serialize(script, stream);

		serialize.NumberI32_Unbounded("x", -123);
		serialize.NumberU32_Unbounded("y", 1234);
		serialize.NumberI32("z", 12345, 0, 65535);

		TS_ASSERT_STREAM(stream, 12, "\x85\xff\xff\xff" "\xd2\x04\x00\x00" "\x39\x30\x00\x00");

		CStdDeserializer deserialize(script, stream);
		int32_t n;

		deserialize.NumberI32_Unbounded(n);
		TS_ASSERT_EQUALS(n, -123);
		deserialize.NumberI32_Unbounded(n);
		TS_ASSERT_EQUALS(n, 1234);
		deserialize.NumberI32(n, 0, 65535);
		TS_ASSERT_EQUALS(n, 12345);

		TS_ASSERT(stream.good());
		TS_ASSERT_EQUALS(stream.peek(), EOF);
	}

	void test_Std_types()
	{
		ScriptInterface script("Test");
		std::stringstream stream;
		CStdSerializer serialize(script, stream);

		serialize_types(serialize);

		CStdDeserializer deserialize(script, stream);
		uint8_t u8v;
		int32_t i32v;
		uint32_t u32v;
		float flt;
		double dbl;
		CFixed_23_8 fixed;
		bool bl;
		std::string str;
		std::wstring wstr;
		u8 cbuf[256];

		deserialize.NumberU8_Unbounded(u8v);
		TS_ASSERT_EQUALS(u8v, 255);
		deserialize.NumberI32_Unbounded(i32v);
		TS_ASSERT_EQUALS(i32v, -123);
		deserialize.NumberU32_Unbounded(u32v);
		TS_ASSERT_EQUALS(u32v, 4294967173);
		deserialize.NumberFloat_Unbounded(flt);
		TS_ASSERT_EQUALS(flt, 1e+30f);
		deserialize.NumberDouble_Unbounded(dbl);
		TS_ASSERT_EQUALS(dbl, 1e+300);
		deserialize.NumberFixed_Unbounded(fixed);
		TS_ASSERT_EQUALS(fixed.ToDouble(), 1234.5);

		deserialize.Bool(bl);
		TS_ASSERT_EQUALS(bl, true);
		deserialize.Bool(bl);
		TS_ASSERT_EQUALS(bl, false);

		deserialize.StringASCII(str, 0, 255);
		TS_ASSERT_STR_EQUALS(str, "example");
		deserialize.StringASCII(str, 0, 255);
		TS_ASSERT_STR_EQUALS(str, "example\"\\\"");
		deserialize.StringASCII(str, 0, 255);
		TS_ASSERT_STR_EQUALS(str, "example\n\ntest");

		wchar_t testw[] = { 't', 0xEA, 's', 't', 0 };
		deserialize.String(wstr, 0, 255);
		TS_ASSERT_WSTR_EQUALS(wstr, testw);

		cbuf[6] = 0x42; // sentinel
		deserialize.RawBytes(cbuf, 6);
		TS_ASSERT_SAME_DATA(cbuf, (const u8*)"\0\1\2\3\x0f\x10\x42", 7);

		TS_ASSERT(stream.good());
		TS_ASSERT_EQUALS(stream.peek(), EOF);
	}

	void test_Hash_basic()
	{
		ScriptInterface script("Test");
		CHashSerializer serialize(script);

		serialize.NumberI32_Unbounded("x", -123);
		serialize.NumberU32_Unbounded("y", 1234);
		serialize.NumberI32("z", 12345, 0, 65535);

		TS_ASSERT_EQUALS(serialize.GetHashLength(), (size_t)16);
		TS_ASSERT_SAME_DATA(serialize.ComputeHash(), "\xa0\x3a\xe5\x3e\x9b\xd7\xfb\x11\x88\x35\xc6\xfb\xb9\x94\xa9\x72", 16);
		// echo -en "\x85\xff\xff\xff\xd2\x04\x00\x00\x39\x30\x00\x00" | openssl md5 | perl -pe 's/(..)/\\x$1/g'
	}

	void test_bounds()
	{
		ScriptInterface script("Test");
		std::stringstream stream;
		CDebugSerializer serialize(script, stream);
		serialize.NumberI32("x", 16, -16, 16);
		serialize.NumberI32("x", -16, -16, 16);
		TS_ASSERT_THROWS(serialize.NumberI32("x", 17, -16, 16), PSERROR_Serialize_OutOfBounds);
		TS_ASSERT_THROWS(serialize.NumberI32("x", -17, -16, 16), PSERROR_Serialize_OutOfBounds);
	}

	// TODO: test exceptions more thoroughly

	void test_script_basic()
	{
		ScriptInterface script("Test");
		CScriptVal obj;
		TS_ASSERT(script.Eval("({'x': 123, 'y': [1, 1.5, '2', 'test', undefined, null, true, false]})", obj));

		{
			std::stringstream stream;
			CDebugSerializer serialize(script, stream);
			serialize.ScriptVal("script", obj);
			TS_ASSERT_STR_EQUALS(stream.str(), "script: ({x:123, y:[1, 1.5, \"2\", \"test\", (void 0), null, true, false]})\n");
		}

		{
			std::stringstream stream;
			CStdSerializer serialize(script, stream);

			serialize.ScriptVal("script", obj);

			TS_ASSERT_STREAM(stream, 100,
					"\x03" // SCRIPT_TYPE_OBJECT
					"\x02\0\0\0" // num props
					"\x01\0\0\0" "x" // "x"
					"\x05" // SCRIPT_TYPE_INT
					"\x7b\0\0\0" // 123
					"\x01\0\0\0" "y" // "y"
					"\x02" // SCRIPT_TYPE_ARRAY
					"\x08\0\0\0" // num props
					"\x01\0\0\0" "0" // "0"
					"\x05" "\x01\0\0\0" // SCRIPT_TYPE_INT 1
					"\x01\0\0\0" "1" // "1"
					"\x06" "\0\0\0\0\0\0\xf8\x3f" // SCRIPT_TYPE_DOUBLE 1.5
					"\x01\0\0\0" "2" // "2"
					"\x04" "\x01\0\0\0" "2" // SCRIPT_TYPE_STRING "2"
					"\x01\0\0\0" "3" // "3"
					"\x04" "\x04\0\0\0" "test" // SCRIPT_TYPE_STRING "test"
					"\x01\0\0\0" "4" // "4"
					"\x00" // SCRIPT_TYPE_VOID
					"\x01\0\0\0" "5" // "5"
					"\x01" // SCRIPT_TYPE_NULL
					"\x01\0\0\0" "6" // "6"
					"\x07" "\x01" // SCRIPT_TYPE_BOOLEAN true
					"\x01\0\0\0" "7" // "7"
					"\x07" "\x00" // SCRIPT_TYPE_BOOLEAN false
			);

			CStdDeserializer deserialize(script, stream);

			jsval newobj;
			deserialize.ScriptVal(newobj);
			TS_ASSERT(stream.good());
			TS_ASSERT_EQUALS(stream.peek(), EOF);

			std::string source;
			TS_ASSERT(script.CallFunction(newobj, "toSource", source));
			TS_ASSERT_STR_EQUALS(source, "({x:123, y:[1, 1.5, \"2\", \"test\", (void 0), null, true, false]})");
		}
	}

	void helper_script_roundtrip(const char* msg, const char* input, const char* expected, size_t expstreamlen = 0, const char* expstream = NULL)
	{
		ScriptInterface script("Test");
		CScriptVal obj;
		TSM_ASSERT(msg, script.Eval(input, obj));

		std::stringstream stream;
		CStdSerializer serialize(script, stream);

		serialize.ScriptVal("script", obj);

		if (expstream)
		{
			TSM_ASSERT_STREAM(msg, stream, expstreamlen, expstream);
		}

		CStdDeserializer deserialize(script, stream);

		jsval newobj;
		deserialize.ScriptVal(newobj);
		TSM_ASSERT(msg, stream.good());
		TSM_ASSERT_EQUALS(msg, stream.peek(), EOF);

		std::string source;
		TSM_ASSERT(msg, script.CallFunction(newobj, "toSource", source));
		TS_ASSERT_STR_EQUALS(source, expected);
	}

	void test_script_unicode()
	{
		helper_script_roundtrip("unicode", "({"
			"'x': \"\\x01\\x80\\xff\\u0100\\ud7ff\", "
			"'y': \"\\ue000\\ufffd\""
			"})",
		/* expected: */
		"({"
			"x:\"\\x01\\x80\\xFF\\u0100\\uD7FF\", "
			"y:\"\\uE000\\uFFFD\""
			"})");

		TS_ASSERT_THROWS(helper_script_roundtrip("invalid chars 1", "(\"\\ud7ff\\ud800\")", "..."), PSERROR_Serialize_InvalidCharInString);
		TS_ASSERT_THROWS(helper_script_roundtrip("invalid chars 2", "(\"\\udfff\")", "..."), PSERROR_Serialize_InvalidCharInString);
		TS_ASSERT_THROWS(helper_script_roundtrip("invalid chars 3", "(\"\\uffff\")", "..."), PSERROR_Serialize_InvalidCharInString);
		TS_ASSERT_THROWS(helper_script_roundtrip("invalid chars 4", "(\"\\ud800\\udc00\")" /* U+10000 */, "..."), PSERROR_Serialize_InvalidCharInString);
	}

	void TODO_test_script_objects()
	{
		helper_script_roundtrip("Number", "([1, new Number('2.0'), 3])", "([1, new Number(2), 3])");
		helper_script_roundtrip("Number with props", "var n=new Number('2.0'); n.foo='bar'; n", "(new Number(2))");
	}

	void test_script_property_order()
	{
		helper_script_roundtrip("prop order 1", "var x={}; x.a=1; x.f=2; x.b=7; x.d=3; x", "({a:1, f:2, b:7, d:3})");
		helper_script_roundtrip("prop order 2", "var x={}; x.d=3; x.a=1; x.f=2; x.b=7; x", "({d:3, a:1, f:2, b:7})");
	}

	void test_script_numbers()
	{
		const char stream[] = "\x02" // SCRIPT_TYPE_ARRAY
					"\x04\0\0\0" // num props
					"\x01\0\0\0" "0" // "0"
					"\x05" "\x00\0\0\xC0" // SCRIPT_TYPE_INT -1073741824 (JS_INT_MIN)
					"\x01\0\0\0" "1" // "1"
					"\x06" "\0\0\x40\0\0\0\xD0\xC1" // SCRIPT_TYPE_DOUBLE -1073741825 (JS_INT_MIN-1)
					"\x01\0\0\0" "2" // "2"
					"\x05" "\xFF\xFF\xFF\x3F" // SCRIPT_TYPE_INT 1073741823
					"\x01\0\0\0" "3" // "3"
					"\x06" "\0\0\0\0\0\0\xD0\x41" // SCRIPT_TYPE_DOUBLE 1073741824
		;

		helper_script_roundtrip("numbers", "[-1073741824, -1073741825, 1.073741823e+9, 1073741824]",
				"[-1073741824, -1073741825, 1073741823, 1073741824]", sizeof(stream) - 1, stream);
	}

	void test_script_exceptions()
	{
		ScriptInterface script("Test");
		CScriptVal obj;

		std::stringstream stream;
		CStdSerializer serialize(script, stream);

		TestLogger logger;

		TS_ASSERT(script.Eval("({x:1, y:<x/>})", obj));
		TS_ASSERT_THROWS(serialize.ScriptVal("script", obj), PSERROR_Serialize_InvalidScriptValue);

		TS_ASSERT(script.Eval("([1, 2, function () { }])", obj));
		TS_ASSERT_THROWS(serialize.ScriptVal("script", obj), PSERROR_Serialize_InvalidScriptValue);
	}

	// TODO: test deserializing invalid streams

	// TODO: test non-tree script structures
	// (not critical since TestComponentManager::test_script_serialization indirectly tests that already)
};

class TestSerializerPerf : public CxxTest::TestSuite
{
public:
	void DISABLED_test_script_props()
	{
		const char* input = "var x = {}; for (var i=0;i<256;++i) x[i]=Math.pow(i, 2); x";

		ScriptInterface script("Test");
		CScriptVal obj;
		TS_ASSERT(script.Eval(input, obj));

		for (size_t i = 0; i < 256; ++i)
		{
			std::stringstream stream;
			CStdSerializer serialize(script, stream);

			serialize.ScriptVal("script", obj);

			CStdDeserializer deserialize(script, stream);

			jsval newobj;
			deserialize.ScriptVal(newobj);
			TS_ASSERT(stream.good());
			TS_ASSERT_EQUALS(stream.peek(), EOF);

			if (i == 0)
			{
				std::string source;
				TS_ASSERT(script.CallFunction(newobj, "toSource", source));
				std::cout << source << "\n";
			}
		}
	}
};
