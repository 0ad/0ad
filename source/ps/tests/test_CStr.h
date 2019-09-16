/* Copyright (C) 2019 Wildfire Games.
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

#include "ps/CStr.h"

class TestCStr : public CxxTest::TestSuite
{
public:
	void test_utf8_utf16_conversion()
	{
		const wchar_t chr_utf16[] = {
			0x12,
			0xff,
			0x1234,
			0x3456,
			0x5678,
			0x7890,
			0x9abc,
			0xbcde,
			0xfffd
		};
		const unsigned char chr_utf8[] = {
			0x12,
			0xc3, 0xbf,
			0xe1, 0x88, 0xb4,
			0xe3, 0x91, 0x96,
			0xe5, 0x99, 0xb8,
			0xe7, 0xa2, 0x90,
			0xe9, 0xaa, 0xbc,
			0xeb, 0xb3, 0x9e,
			0xef, 0xbf, 0xbd
		};
		CStrW str_utf16 (chr_utf16, sizeof(chr_utf16)/sizeof(wchar_t));

		CStr8 str_utf8 = str_utf16.ToUTF8();
		TS_ASSERT_EQUALS(str_utf8.length(), sizeof(chr_utf8));
		TS_ASSERT_SAME_DATA(str_utf8.data(), chr_utf8, sizeof(chr_utf8));

		CStrW str_utf16b = str_utf8.FromUTF8();
		TS_ASSERT_WSTR_EQUALS(str_utf16b, str_utf16);
	}

	void test_invalid_utf8()
	{
		struct { const char* utf8; const wchar_t* utf16; } tests[] = {
			{ "a\xef", L"a\xfffd" },
			{ "b\xef\xbf", L"b\xfffd\xfffd" },
			{ "c\xef\xbf\x01", L"c\xfffd\xfffd\x0001" },
			{ "d\xffX\x80Y\x80" , L"d\xfffdX\xfffdY\xfffd" }
		};
		for (size_t i = 0; i < ARRAY_SIZE(tests); ++i)
		{
			CStr8 str_utf8 (tests[i].utf8);
			CStrW str_utf16 (tests[i].utf16);

			CStrW str_utf8to16 = str_utf8.FromUTF8();
			TS_ASSERT_EQUALS(str_utf16.length(), str_utf8to16.length());
			TS_ASSERT_SAME_DATA(str_utf8to16.data(), str_utf16.data(), str_utf16.length()*sizeof(wchar_t));
		}
	}

	template <typename T>
	void roundtrip(const T& str)
	{
		size_t len = str.GetSerializedLength();
		u8* buf = new u8[len+1];
		buf[len] = '!';
		TS_ASSERT_EQUALS(str.Serialize(buf) - (buf+len), 0);
		TS_ASSERT_EQUALS(buf[len], '!');

		T str2;
		TS_ASSERT_EQUALS(str2.Deserialize(buf, buf+len) - (buf+len), 0);
		TS_ASSERT_EQUALS(str2.length(), str.length());
		TS_ASSERT_EQUALS(str2, str);

		T str3;
		TS_ASSERT_EQUALS(str3.Deserialize(buf, buf+len+256) - (buf+len), 0);
		TS_ASSERT_EQUALS(str3.length(), str.length());
		TS_ASSERT_EQUALS(str3, str);

		delete[] buf;
	}

	void test_serialize_8()
	{
		CStr8 str1("hello");
		roundtrip(str1);

		CStr8 str2 = str1;
		str2[3] = '\0';
		roundtrip(str2);
	}

	void test_parse()
	{
		// Parsing should be independent of locale ("," vs "."),
		// because GTK+ can change the locale when we're running Atlas.
		// (If the host system doesn't have the locale we're using for this test
		// then it'll just stick with the default, which is fine)
		char* old = setlocale(LC_NUMERIC, "fr_FR.UTF-8");

		CStr8 str1("1.234");
		TS_ASSERT_DELTA(str1.ToFloat(), 1.234f, 0.0001f);
		TS_ASSERT_DELTA(str1.ToDouble(), 1.234, 0.0001);
		TS_ASSERT_EQUALS(str1.ToInt(), 1);
		TS_ASSERT_EQUALS(str1.ToUInt(), 1u);
		TS_ASSERT_EQUALS(str1.ToLong(), 1);
		TS_ASSERT_EQUALS(str1.ToULong(), 1u);

		CStr8 str2("+1,234");
		TS_ASSERT_DELTA(str2.ToFloat(), 1.f, 0.0001f);
		TS_ASSERT_DELTA(str2.ToDouble(), 1., 0.0001);
		TS_ASSERT_EQUALS(str2.ToInt(), 1);
		TS_ASSERT_EQUALS(str2.ToUInt(), 1u);
		TS_ASSERT_EQUALS(str2.ToLong(), 1);
		TS_ASSERT_EQUALS(str2.ToULong(), 1u);

		CStr8 str3("bogus");
		TS_ASSERT_EQUALS(str3.ToFloat(), 0.0f);
		TS_ASSERT_EQUALS(str3.ToDouble(), 0.0);
		TS_ASSERT_EQUALS(str3.ToInt(), 0);
		TS_ASSERT_EQUALS(str3.ToUInt(), 0u);
		TS_ASSERT_EQUALS(str3.ToLong(), 0);
		TS_ASSERT_EQUALS(str3.ToULong(), 0u);

		CStr8 str4("3bogus");
		TS_ASSERT_DELTA(str4.ToFloat(), 3.0f, 0.0001f);
		TS_ASSERT_DELTA(str4.ToDouble(), 3.0, 0.0001);
		TS_ASSERT_EQUALS(str4.ToInt(), 3);
		TS_ASSERT_EQUALS(str4.ToUInt(), 3u);
		TS_ASSERT_EQUALS(str4.ToLong(), 3);
		TS_ASSERT_EQUALS(str4.ToULong(), 3u);

		CStr8 str5("-3bogus");
		TS_ASSERT_DELTA(str5.ToFloat(), -3.0f, 0.0001f);
		TS_ASSERT_DELTA(str5.ToDouble(), -3.0, 0.0001);
		TS_ASSERT_EQUALS(str5.ToInt(), -3);
		TS_ASSERT_EQUALS(str5.ToLong(), -3);

		setlocale(LC_NUMERIC, old);
	}
};
