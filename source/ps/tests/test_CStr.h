#include <cxxtest/TestSuite.h>

#include "ps/CStr.h"

class TestCStr : public CxxTest::TestSuite 
{
public:
	void test_utf8_utf16_conversion()
	{
		const wchar_t chr_utf16[] = { 0x12, 0xff, 0x1234, 0x3456, 0x5678, 0x7890, 0x9abc, 0xbcde, 0xfffe };
		const unsigned char chr_utf8[] = { 0x12, 0xc3, 0xbf, 0xe1, 0x88, 0xb4, 0xe3, 0x91, 0x96, 0xe5, 0x99, 0xb8, 0xe7, 0xa2, 0x90, 0xe9, 0xaa, 0xbc, 0xeb, 0xb3, 0x9e, 0xef, 0xbf, 0xbe };
		CStrW str_utf16 (chr_utf16, sizeof(chr_utf16)/sizeof(wchar_t));
		CStr8 str_utf8 = str_utf16.ToUTF8();
		TS_ASSERT_EQUAL(str_utf8.length(), sizeof(chr_utf8));
		TS_ASSERT_SAME_DATA(str_utf8.data(), chr_utf8, sizeof(chr_utf8));
		TS_ASSERT_EQUAL(str_utf8.FromUTF8(), str_utf16);
	}
};
