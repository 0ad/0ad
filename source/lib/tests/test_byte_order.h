#include "lib/precompiled.h"

#include <lib/byte_order.h>

#include <cxxtest/TestSuite.h>

class TestByteOrder : public CxxTest::TestSuite 
{
public:
	void test_()
	{
		const u32 x = 0x01234567u;
		const u8 LS_byte = *(u8*)&x;
		// little endian
		if(LS_byte == 0x67)
		{
			TS_ASSERT_EQUALS(to_le16(0x0123u), 0x0123u);
			TS_ASSERT_EQUALS(to_le32(0x01234567u), 0x01234567u);
			TS_ASSERT_EQUALS(to_le64(0x0123456789ABCDEFULL), 0x0123456789ABCDEFULL);

			TS_ASSERT_EQUALS(to_be16(0x0123u), 0x2301u);
			TS_ASSERT_EQUALS(to_be32(0x01234567u), 0x67452301u);
			TS_ASSERT_EQUALS(to_be64(0x0123456789ABCDEFull), 0xEFCDAB8967452301ull);
		}
		// big endian
		else if(LS_byte == 0x01)
		{
			TS_ASSERT_EQUALS(to_le16(0x0123u), 0x2301u);
			TS_ASSERT_EQUALS(to_le32(0x01234567u), 0x67452301u);
			TS_ASSERT_EQUALS(to_le64(0x0123456789ABCDEFull), 0xEFCDAB8967452301ull);

			TS_ASSERT_EQUALS(to_be16(0x0123u), 0x0123u);
			TS_ASSERT_EQUALS(to_be32(0x01234567u), 0x01234567u);
			TS_ASSERT_EQUALS(to_be64(0x0123456789ABCDEFull), 0x0123456789ABCDEFull);
		}
		else
			TS_FAIL("endian determination failed");

		// note: no need to test read_?e* / write_?e* - they are
		// trivial wrappers on top of to_?e*.
	}
};
