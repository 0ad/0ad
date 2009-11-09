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

#include "lib/self_test.h"
#include "lib/wchar.h"

// (copied from CStr test)

class Test_wchar : public CxxTest::TestSuite 
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
		const std::wstring str_utf16(chr_utf16, ARRAY_SIZE(chr_utf16));

		const std::string str_utf8 = utf8_from_wstring(str_utf16);
		TS_ASSERT_EQUALS(str_utf8.length(), ARRAY_SIZE(chr_utf8));
		TS_ASSERT_SAME_DATA(str_utf8.data(), chr_utf8, ARRAY_SIZE(chr_utf8)*sizeof(char));

		const std::wstring str_utf16b = wstring_from_utf8(str_utf8);
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
			const std::string str_utf8(tests[i].utf8);
			const std::wstring str_utf16(tests[i].utf16);

			const std::wstring str_utf8to16 = wstring_from_utf8(str_utf8);
			TS_ASSERT_EQUALS(str_utf16.length(), str_utf8to16.length());
			TS_ASSERT_SAME_DATA(str_utf8to16.data(), str_utf16.data(), str_utf16.length()*sizeof(wchar_t));
		}
	}
};
