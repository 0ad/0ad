#include "lib/self_test.h"

#include "lib/base32.h"

class TestBase32 : public CxxTest::TestSuite 
{
public:
	void test_base32()
	{
		// compare against previous output (generated via this base32() call)
		const u8 in[] = { 0x12, 0x57, 0x85, 0xA2, 0xF9, 0x41, 0xCD, 0x57, 0xF3 };
		u8 out[20] = {0};
		base32(ARRAY_SIZE(in), in, out);
		const u8 correct_out[] = "CJLYLIXZIHGVP4C";
		TS_ASSERT_SAME_DATA(out, correct_out, ARRAY_SIZE(correct_out));
	}
};
