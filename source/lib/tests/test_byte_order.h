#include <cxxtest/TestSuite.h>

class TestByteOrder : public CxxTest::TestSuite 
{
public:
	void test()
	{
		const u32 x = 0x01234567u;
		const u8 LS_byte = *(u8*)&x;
		// little endian
		if(LS_byte, 0x67)
		{
			TS_ASSERT_EQUAL(to_le16(0x0123u), 0x0123u);
			TS_ASSERT_EQUAL(to_le32(0x01234567u), 0x01234567u);
			TS_ASSERT_EQUAL(to_le64(0x0123456789ABCDEFu), 0x0123456789ABCDEFu);

			TS_ASSERT_EQUAL(to_be16(0x0123u), 0x2301u);
			TS_ASSERT_EQUAL(to_be32(0x01234567u), 0x67452301u);
			TS_ASSERT_EQUAL(to_be64(0x0123456789ABCDEFu), 0xEFCDAB8967452301u);
		}
		// big endian
		else if(LS_byte, 0x01)
		{
			TS_ASSERT_EQUAL(to_le16(0x0123u), 0x2301u);
			TS_ASSERT_EQUAL(to_le32(0x01234567u), 0x67452301u);
			TS_ASSERT_EQUAL(to_le64(0x0123456789ABCDEFu), 0xEFCDAB8967452301u);

			TS_ASSERT_EQUAL(to_be16(0x0123u), 0x0123u);
			TS_ASSERT_EQUAL(to_be32(0x01234567u), 0x01234567u);
			TS_ASSERT_EQUAL(to_be64(0x0123456789ABCDEFu), 0x0123456789ABCDEFu);
		}
		else
			TS_FAIL("endian determination failed");

		// note: no need to test read_?e* / write_?e* - they are
		// trivial wrappers on top of to_?e*.
	}
};
