/* Copyright (C) 2013 Wildfire Games.
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

#include "graphics/MapReader.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureManager.h"
#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Loader.h"
#include "ps/XML/Xeromyces.h"
#include "simulation2/Simulation2.h"

#include "callgrind.h"

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
		serialize.NumberI8_Unbounded("i8", (signed char)-123);
		serialize.NumberU8_Unbounded("u8", (unsigned char)255);
		serialize.NumberI16_Unbounded("i16", -12345);
		serialize.NumberU16_Unbounded("u16", 56789);
		serialize.NumberI32_Unbounded("i32", -123);
		serialize.NumberU32_Unbounded("u32", (unsigned)-123);
		serialize.NumberFloat_Unbounded("float", 1e+30f);
		serialize.NumberDouble_Unbounded("double", 1e+300);
		serialize.NumberFixed_Unbounded("fixed", fixed::FromFloat(1234.5f));

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
		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
		std::stringstream stream;
		CDebugSerializer serialize(script, stream);
		serialize.NumberI32_Unbounded("x", -123);
		serialize.NumberU32_Unbounded("y", 1234);
		serialize.NumberI32("z", 12345, 0, 65535);
		TS_ASSERT_STR_EQUALS(stream.str(), "x: -123\ny: 1234\nz: 12345\n");
	}

	void test_Debug_floats()
	{
		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
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
		serialize.NumberFixed_Unbounded("x", fixed::FromDouble(1e4));
		TS_ASSERT_STR_EQUALS(stream.str(),
				"x: 10000\nx: 9.9999997e-05\nx: 100000\nx: 9.9999997e-06\nx: 1000000\nx: 1e-06\nx: 1e+10\nx: 1e-10\n"
				"x: 10000\nx: 0.0001\nx: 100000\nx: 1.0000000000000001e-05\nx: 1000000\nx: 9.9999999999999995e-07\nx: 10000000000\nx: 1e-10\nx: 1e+100\nx: 1e-100\n"
				"x: 10000\n"
		);
	}

	void test_Debug_types()
	{
		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
		std::stringstream stream;
		CDebugSerializer serialize(script, stream);

		serialize.Comment("comment");

		serialize_types(serialize);

		TS_ASSERT_STR_EQUALS(stream.str(),
				"# comment\n"
				"i8: -123\n"
				"u8: 255\n"
				"i16: -12345\n"
				"u16: 56789\n"
				"i32: -123\n"
				"u32: 4294967173\n"
				"float: 1e+30\n"
				"double: 1.0000000000000001e+300\n"
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
		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
		std::stringstream stream;
		CStdSerializer serialize(script, stream);

		serialize.NumberI32_Unbounded("x", -123);
		serialize.NumberU32_Unbounded("y", 1234);
		serialize.NumberI32("z", 12345, 0, 65535);

		TS_ASSERT_STREAM(stream, 12, "\x85\xff\xff\xff" "\xd2\x04\x00\x00" "\x39\x30\x00\x00");

		CStdDeserializer deserialize(script, stream);
		int32_t n;

		deserialize.NumberI32_Unbounded("x", n);
		TS_ASSERT_EQUALS(n, -123);
		deserialize.NumberI32_Unbounded("y", n);
		TS_ASSERT_EQUALS(n, 1234);
		deserialize.NumberI32("z", n, 0, 65535);
		TS_ASSERT_EQUALS(n, 12345);

		TS_ASSERT(stream.good());
		TS_ASSERT_EQUALS(stream.peek(), EOF);
	}

	void test_Std_types()
	{
		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
		std::stringstream stream;
		CStdSerializer serialize(script, stream);

		serialize_types(serialize);

		CStdDeserializer deserialize(script, stream);
		int8_t i8v;
		uint8_t u8v;
		int16_t i16v;
		uint16_t u16v;
		int32_t i32v;
		uint32_t u32v;
		float flt;
		double dbl;
		fixed fxd;
		bool bl;
		std::string str;
		std::wstring wstr;
		u8 cbuf[256];

		deserialize.NumberI8_Unbounded("i8", i8v);
		TS_ASSERT_EQUALS(i8v, -123);
		deserialize.NumberU8_Unbounded("u8", u8v);
		TS_ASSERT_EQUALS(u8v, 255);
		deserialize.NumberI16_Unbounded("i16", i16v);
		TS_ASSERT_EQUALS(i16v, -12345);
		deserialize.NumberU16_Unbounded("u16", u16v);
		TS_ASSERT_EQUALS(u16v, 56789);
		deserialize.NumberI32_Unbounded("i32", i32v);
		TS_ASSERT_EQUALS(i32v, -123);
		deserialize.NumberU32_Unbounded("u32", u32v);
		TS_ASSERT_EQUALS(u32v, 4294967173u);
		deserialize.NumberFloat_Unbounded("float", flt);
		TS_ASSERT_EQUALS(flt, 1e+30f);
		deserialize.NumberDouble_Unbounded("double", dbl);
		TS_ASSERT_EQUALS(dbl, 1e+300);
		deserialize.NumberFixed_Unbounded("fixed", fxd);
		TS_ASSERT_EQUALS(fxd.ToDouble(), 1234.5);

		deserialize.Bool("t", bl);
		TS_ASSERT_EQUALS(bl, true);
		deserialize.Bool("f", bl);
		TS_ASSERT_EQUALS(bl, false);

		deserialize.StringASCII("string", str, 0, 255);
		TS_ASSERT_STR_EQUALS(str, "example");
		deserialize.StringASCII("string 2", str, 0, 255);
		TS_ASSERT_STR_EQUALS(str, "example\"\\\"");
		deserialize.StringASCII("string 3", str, 0, 255);
		TS_ASSERT_STR_EQUALS(str, "example\n\ntest");

		wchar_t testw[] = { 't', 0xEA, 's', 't', 0 };
		deserialize.String("string 4", wstr, 0, 255);
		TS_ASSERT_WSTR_EQUALS(wstr, testw);

		cbuf[6] = 0x42; // sentinel
		deserialize.RawBytes("raw bytes", cbuf, 6);
		TS_ASSERT_SAME_DATA(cbuf, (const u8*)"\0\1\2\3\x0f\x10\x42", 7);

		TS_ASSERT(stream.good());
		TS_ASSERT_EQUALS(stream.peek(), EOF);
	}

	void test_Hash_basic()
	{
		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
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
		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
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
		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
		CScriptVal obj;
		TS_ASSERT(script.Eval("({'x': 123, 'y': [1, 1.5, '2', 'test', undefined, null, true, false]})", obj));

		{
			std::stringstream stream;
			CDebugSerializer serialize(script, stream);
			serialize.ScriptVal("script", obj);
			TS_ASSERT_STR_EQUALS(stream.str(),
					"script: {\n"
					"  \"x\": 123,\n"
					"  \"y\": [\n"
					"    1,\n"
					"    1.5,\n"
					"    \"2\",\n"
					"    \"test\",\n"
					"    null,\n"
					"    null,\n"
					"    true,\n"
					"    false\n"
					"  ]\n"
					"}\n");
		}

		{
			std::stringstream stream;
			CStdSerializer serialize(script, stream);

			serialize.ScriptVal("script", obj);

			TS_ASSERT_STREAM(stream, 119,
					"\x03" // SCRIPT_TYPE_OBJECT
					"\x02\0\0\0" // num props
					"\x01\0\0\0" "x\0" // "x"
					"\x05" // SCRIPT_TYPE_INT
					"\x7b\0\0\0" // 123
					"\x01\0\0\0" "y\0" // "y"
					"\x02" // SCRIPT_TYPE_ARRAY
					"\x08\0\0\0" // array length
					"\x08\0\0\0" // num props
					"\x01\0\0\0" "0\0" // "0"
					"\x05" "\x01\0\0\0" // SCRIPT_TYPE_INT 1
					"\x01\0\0\0" "1\0" // "1"
					"\x06" "\0\0\0\0\0\0\xf8\x3f" // SCRIPT_TYPE_DOUBLE 1.5
					"\x01\0\0\0" "2\0" // "2"
					"\x04" "\x01\0\0\0" "2\0" // SCRIPT_TYPE_STRING "2"
					"\x01\0\0\0" "3\0" // "3"
					"\x04" "\x04\0\0\0" "t\0e\0s\0t\0" // SCRIPT_TYPE_STRING "test"
					"\x01\0\0\0" "4\0" // "4"
					"\x00" // SCRIPT_TYPE_VOID
					"\x01\0\0\0" "5\0" // "5"
					"\x01" // SCRIPT_TYPE_NULL
					"\x01\0\0\0" "6\0" // "6"
					"\x07" "\x01" // SCRIPT_TYPE_BOOLEAN true
					"\x01\0\0\0" "7\0" // "7"
					"\x07" "\x00" // SCRIPT_TYPE_BOOLEAN false
			);

			CStdDeserializer deserialize(script, stream);

			jsval newobj;
			deserialize.ScriptVal("script", newobj);
			TS_ASSERT(stream.good());
			TS_ASSERT_EQUALS(stream.peek(), EOF);

			std::string source;
			TS_ASSERT(script.CallFunction(newobj, "toSource", source));
			TS_ASSERT_STR_EQUALS(source, "({x:123, y:[1, 1.5, \"2\", \"test\", (void 0), null, true, false]})");
		}
	}

	void helper_script_roundtrip(const char* msg, const char* input, const char* expected, size_t expstreamlen = 0, const char* expstream = NULL)
	{
		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
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
		deserialize.ScriptVal("script", newobj);
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

		// Disabled since we no longer do the UTF-8 conversion that rejects invalid characters
//		TS_ASSERT_THROWS(helper_script_roundtrip("invalid chars 1", "(\"\\ud7ff\\ud800\")", "..."), PSERROR_Serialize_InvalidCharInString);
//		TS_ASSERT_THROWS(helper_script_roundtrip("invalid chars 2", "(\"\\udfff\")", "..."), PSERROR_Serialize_InvalidCharInString);
//		TS_ASSERT_THROWS(helper_script_roundtrip("invalid chars 3", "(\"\\uffff\")", "..."), PSERROR_Serialize_InvalidCharInString);
//		TS_ASSERT_THROWS(helper_script_roundtrip("invalid chars 4", "(\"\\ud800\\udc00\")" /* U+10000 */, "..."), PSERROR_Serialize_InvalidCharInString);
		helper_script_roundtrip("unicode", "\"\\ud800\\uffff\"", "(new String(\"\\uD800\\uFFFF\"))");
	}

	void test_script_objects()
	{
		helper_script_roundtrip("Number", "[1, new Number('2.0'), 3]", "[1, (new Number(2)), 3]");
		helper_script_roundtrip("Number with props", "var n=new Number('2.0'); n.foo='bar'; n", "(new Number(2))");

		helper_script_roundtrip("String", "['test1', new String('test2'), 'test3']", "[\"test1\", (new String(\"test2\")), \"test3\"]");
		helper_script_roundtrip("String with props", "var s=new String('test'); s.foo='bar'; s", "(new String(\"test\"))");

		helper_script_roundtrip("Boolean", "[new Boolean('true'), false]", "[(new Boolean(true)), false]");
		helper_script_roundtrip("Boolean with props", "var b=new Boolean('true'); b.foo='bar'; b", "(new Boolean(true))");
	}

	void test_script_typed_arrays_simple()
	{
		helper_script_roundtrip("Int8Array",
			"var arr=new Int8Array(8);"
			"for(var i=0; i<arr.length; ++i)"
			"  arr[i]=(i+1)*32;"
			"arr",
		/* expected: */
			"({0:32, 1:64, 2:96, 3:-128, 4:-96, 5:-64, 6:-32, 7:0})"
		);

		helper_script_roundtrip("Uint8Array",
			"var arr=new Uint8Array(8);"
			"for(var i=0; i<arr.length; ++i)"
			"  arr[i]=(i+1)*32;"
			"arr",
		/* expected: */
			"({0:32, 1:64, 2:96, 3:128, 4:160, 5:192, 6:224, 7:0})"
		);

		helper_script_roundtrip("Int16Array",
			"var arr=new Int16Array(8);"
			"for(var i=0; i<arr.length; ++i)"
			"  arr[i]=(i+1)*8192;"
			"arr",
		/* expected: */
			"({0:8192, 1:16384, 2:24576, 3:-32768, 4:-24576, 5:-16384, 6:-8192, 7:0})"
		);

		helper_script_roundtrip("Uint16Array",
			"var arr=new Uint16Array(8);"
			"for(var i=0; i<arr.length; ++i)"
			"  arr[i]=(i+1)*8192;"
			"arr",
		/* expected: */
			"({0:8192, 1:16384, 2:24576, 3:32768, 4:40960, 5:49152, 6:57344, 7:0})"
		);

		helper_script_roundtrip("Int32Array",
			"var arr=new Int32Array(8);"
			"for(var i=0; i<arr.length; ++i)"
			"  arr[i]=(i+1)*536870912;"
			"arr",
		/* expected: */
			"({0:536870912, 1:1073741824, 2:1610612736, 3:-2147483648, 4:-1610612736, 5:-1073741824, 6:-536870912, 7:0})"
		);

		helper_script_roundtrip("Uint32Array",
			"var arr=new Uint32Array(8);"
			"for(var i=0; i<arr.length; ++i)"
			"  arr[i]=(i+1)*536870912;"
			"arr", 
		/* expected: */
			"({0:536870912, 1:1073741824, 2:1610612736, 3:2147483648, 4:2684354560, 5:3221225472, 6:3758096384, 7:0})"
		);

		helper_script_roundtrip("Float32Array",
			"var arr=new Float32Array(2);"
			"arr[0]=3.4028234e38;"
			"arr[1]=Infinity;"
			"arr", 
		/* expected: */
			"({0:3.4028234663852886e+38, 1:Infinity})"
		);

		helper_script_roundtrip("Float64Array",
			"var arr=new Float64Array(2);"
			"arr[0]=1.7976931348623157e308;"
			"arr[1]=-Infinity;"
			"arr", 
		/* expected: */
			"({0:1.7976931348623157e+308, 1:-Infinity})"
		);

		helper_script_roundtrip("Uint8ClampedArray",
			"var arr=new Uint8ClampedArray(8);"
			"for(var i=0; i<arr.length; ++i)"
			"  arr[i]=(i-2)*64;"
			"arr",
		/* expected: */
			"({0:0, 1:0, 2:0, 3:64, 4:128, 5:192, 6:255, 7:255})"
		);
	}

	void test_script_typed_arrays_complex()
	{
		helper_script_roundtrip("ArrayBuffer with Int16Array",
			"var buf=new ArrayBuffer(16);"
			"var int16=Int16Array(buf);"
			"for(var i=0; i<int16.length; ++i)"
			"  int16[i]=(i+1)*8192;"
			"int16",
		/* expected: */
			"({0:8192, 1:16384, 2:24576, 3:-32768, 4:-24576, 5:-16384, 6:-8192, 7:0})"
		);

		helper_script_roundtrip("ArrayBuffer with Int16Array and Uint32Array",
			"var buf = new ArrayBuffer(16);"
			"var int16 = Int16Array(buf);"
			"for(var i=0; i < int16.length; ++i)"
			"  int16[i] = (i+1)*32768;"
			"var uint32 = new Uint32Array(buf);"
			"uint32[0] = 4294967295;"
			"[int16, uint32]",
		/* expected: */ "["
				"{0:-1, 1:-1, 2:-32768, 3:0, 4:-32768, 5:0, 6:-32768, 7:0}, "
				"{0:4294967295, 1:32768, 2:32768, 3:32768}"
			"]"
		);

		helper_script_roundtrip("ArrayBuffer with complex structure",
			"var buf=new ArrayBuffer(16);" // 16 bytes
			"var chunk1=Int8Array(buf, 0, 4);" // 4 bytes
			"var chunk2=Uint16Array(buf, 4, 2);" // 4 bytes
			"var chunk3=Int32Array(buf, 8, 2);" // 8 bytes
			"for(var i=0; i<chunk1.length; ++i)"
			"  chunk1[i]=255;"
			"for(var i=0; i<chunk2.length; ++i)"
			"  chunk2[i]=65535;"
			"for(var i=0; i<chunk3.length; ++i)"
			"  chunk3[i]=4294967295;"
			"var bytes = Uint8Array(buf);"
			"({'struct':[chunk1, chunk2, chunk3], 'bytes':bytes})",
		/* expected: */ "({"
				"struct:[{0:-1, 1:-1, 2:-1, 3:-1}, {0:65535, 1:65535}, {0:-1, 1:-1}], "
				"bytes:{0:255, 1:255, 2:255, 3:255, 4:255, 5:255, 6:255, 7:255, 8:255, 9:255, 10:255, 11:255, 12:255, 13:255, 14:255, 15:255}"
			"})"
		);
	}

	// TODO: prototype objects

	void test_script_nonfinite()
	{
		helper_script_roundtrip("nonfinite", "[0, Infinity, -Infinity, NaN, -1/Infinity]", "[0, Infinity, -Infinity, NaN, -0]");
	}

	void test_script_property_order()
	{
		helper_script_roundtrip("prop order 1", "var x={}; x.a=1; x.f=2; x.b=7; x.d=3; x", "({a:1, f:2, b:7, d:3})");
		helper_script_roundtrip("prop order 2", "var x={}; x.d=3; x.a=1; x.f=2; x.b=7; x", "({d:3, a:1, f:2, b:7})");
	}

	void test_script_array_sparse()
	{
		helper_script_roundtrip("array_sparse", "[,1,2,,4,,]", "[, 1, 2, , 4, ,]");
	}

	void test_script_numbers()
	{
		const char stream[] = "\x02" // SCRIPT_TYPE_ARRAY
					"\x04\0\0\0" // num props
					"\x04\0\0\0" // array length
					"\x01\0\0\0" "0\0" // "0"
					"\x05" "\0\0\0\x80" // SCRIPT_TYPE_INT -2147483648 (JS_INT_MIN)
					"\x01\0\0\0" "1\0" // "1"
					"\x06" "\0\0\x20\0\0\0\xE0\xC1" // SCRIPT_TYPE_DOUBLE -2147483649 (JS_INT_MIN-1)
					"\x01\0\0\0" "2\0" // "2"
					"\x05" "\xFF\xFF\xFF\x7F" // SCRIPT_TYPE_INT 2147483647 (JS_INT_MAX)
					"\x01\0\0\0" "3\0" // "3"
					"\x06" "\0\0\0\0\0\0\xE0\x41" // SCRIPT_TYPE_DOUBLE 2147483648 (JS_INT_MAX+1)
		;

		helper_script_roundtrip("numbers", "[-2147483648, -2147483649, 2.147483647e+9, 2147483648]",
				"[-2147483648, -2147483649, 2147483647, 2147483648]", sizeof(stream) - 1, stream);
	}

	void test_script_exceptions()
	{
		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
		CScriptVal obj;

		std::stringstream stream;
		CStdSerializer serialize(script, stream);

		TestLogger logger;

		TS_ASSERT(script.Eval("([1, 2, function () { }])", obj));
		TS_ASSERT_THROWS(serialize.ScriptVal("script", obj), PSERROR_Serialize_InvalidScriptValue);
	}

	void test_script_splice()
	{
		helper_script_roundtrip("splice 1", "var a=[10,20]; a.splice(0, 1); a", "[20]");
		helper_script_roundtrip("splice 1", "var a=[10,20]; a.splice(0, 2); a", "[]");
		helper_script_roundtrip("splice 1", "var a=[10,20]; a.splice(0, 0, 5); a", "[5, 10, 20]");
	}

	// TODO: test deserializing invalid streams

	// TODO: test non-tree script structures
	// (not critical since TestComponentManager::test_script_serialization indirectly tests that already)
};

class TestSerializerPerf : public CxxTest::TestSuite
{
public:
	void test_script_props_DISABLED()
	{
		const char* input = "var x = {}; for (var i=0;i<256;++i) x[i]=Math.pow(i, 2); x";

		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
		CScriptVal obj;
		TS_ASSERT(script.Eval(input, obj));

		for (size_t i = 0; i < 256; ++i)
		{
			std::stringstream stream;
			CStdSerializer serialize(script, stream);

			serialize.ScriptVal("script", obj);

			CStdDeserializer deserialize(script, stream);

			jsval newobj;
			deserialize.ScriptVal("script", newobj);
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

	void test_hash_DISABLED()
	{
		CXeromyces::Startup();

		g_VFS = CreateVfs(20 * MiB);
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir()/"mods"/"public", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(g_VFS->Mount(L"cache/", DataDir()/"cache"));

		// Need some stuff for terrain movement costs:
		// (TODO: this ought to be independent of any graphics code)
		new CTerrainTextureManager;
		g_TexMan.LoadTerrainTextures();

		CTerrain terrain;

		CSimulation2 sim2(NULL, ScriptInterface::CreateRuntime(), &terrain);
		sim2.LoadDefaultScripts();
		sim2.ResetState();

		CMapReader* mapReader = new CMapReader(); // it'll call "delete this" itself

		LDR_BeginRegistering();
		mapReader->LoadMap(L"maps/scenarios/Acropolis 01.pmp", CScriptValRooted(), &terrain, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			&sim2, &sim2.GetSimContext(), -1, false);
		LDR_EndRegistering();
		TS_ASSERT_OK(LDR_NonprogressiveLoad());

		sim2.Update(0);

		{
			std::stringstream str;
			std::string hash;
			sim2.SerializeState(str);
			sim2.ComputeStateHash(hash, false);
			debug_printf(L"\n");
			debug_printf(L"# size = %d\n", (int)str.str().length());
			debug_printf(L"# hash = ");
			for (size_t i = 0; i < hash.size(); ++i)
				debug_printf(L"%02x", (unsigned int)(u8)hash[i]);
			debug_printf(L"\n");
		}

		double t = timer_Time();
		CALLGRIND_START_INSTRUMENTATION;
		size_t reps = 128;
		for (size_t i = 0; i < reps; ++i)
		{
			std::string hash;
			sim2.ComputeStateHash(hash, false);
		}
		CALLGRIND_STOP_INSTRUMENTATION;
		t = timer_Time() - t;
		debug_printf(L"# time = %f (%f/%d)\n", t/reps, t, (int)reps);

		// Shut down the world
		delete &g_TexMan;
		g_VFS.reset();
		CXeromyces::Terminate();
	}

};
