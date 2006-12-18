#include "lib/self_test.h"

#include "lib/lib.h"

extern "C"
int
__stdcall
MessageBoxA(
			void*,
			const char*,const char*,
			unsigned int);

class TestLib : public CxxTest::TestSuite 
{
public:
	void test_fnv_hash()
	{
		TS_ASSERT_EQUALS(fnv_hash(""), 0x811C9DC5u);		// verify initial value
		const u32 h1 = fnv_hash("abcdef");
		TS_ASSERT_EQUALS(h1, 0xFF478A2A);					// verify value for simple string
		TS_ASSERT_EQUALS(fnv_hash   ("abcdef", 6), h1);	// same result if hashing buffer
		TS_ASSERT_EQUALS(fnv_lc_hash("ABcDeF", 6), h1);	// same result if case differs

		TS_ASSERT_EQUALS(fnv_hash64(""), 0xCBF29CE484222325ull);	// verify initial value
		const u64 h2 = fnv_hash64("abcdef");
		TS_ASSERT_EQUALS(h2, 0xD80BDA3FBE244A0Aull);		// verify value for simple string
		TS_ASSERT_EQUALS(fnv_hash64("abcdef", 6), h2);	// same result if hashing buffer
	}

	void test_is_pow2()
	{
		TS_ASSERT_EQUALS(is_pow2(0u), false);
		TS_ASSERT_EQUALS(is_pow2(~0u), false);
		TS_ASSERT_EQUALS(is_pow2(0x80000001), false);
		TS_ASSERT_EQUALS(is_pow2(1), true);
		TS_ASSERT_EQUALS(is_pow2(1u << 31), true);
	}

	void test_ilog2()
	{
		TS_ASSERT_EQUALS(ilog2(0u), -1);
		TS_ASSERT_EQUALS(ilog2(3u), -1);
		TS_ASSERT_EQUALS(ilog2(0xffffffffu), -1);
		TS_ASSERT_EQUALS(ilog2(1u), 0);
		TS_ASSERT_EQUALS(ilog2(256u), 8);
		TS_ASSERT_EQUALS(ilog2(0x80000000u), 31);
	}

	void test_log2()
	{
		TS_ASSERT_EQUALS(log2(3u), 2u);
		TS_ASSERT_EQUALS(log2(0xffffffffu), 32u);
		TS_ASSERT_EQUALS(log2(1u), 0u);
		TS_ASSERT_EQUALS(log2(256u), 8u);
		TS_ASSERT_EQUALS(log2(0x80000000u), 31u);
	}

	void test_ilog2f()
	{
		TS_ASSERT_EQUALS(ilog2(1.f), 0);
		TS_ASSERT_EQUALS(ilog2(3.f), 1);
		TS_ASSERT_EQUALS(ilog2(256.f), 8);
	}

	void test_round_next_pow2()
	{
		TS_ASSERT_EQUALS(round_up_to_pow2(0u), 1u);
		TS_ASSERT_EQUALS(round_up_to_pow2(1u), 2u);
		TS_ASSERT_EQUALS(round_up_to_pow2(127u), 128u);
		TS_ASSERT_EQUALS(round_up_to_pow2(128u), 256u);
		TS_ASSERT_EQUALS(round_up_to_pow2(129u), 256u);
	}

	void test_round_up()
	{
		TS_ASSERT_EQUALS(round_up( 0u, 16u), 0u);
		TS_ASSERT_EQUALS(round_up( 4u, 16u), 16u);
		TS_ASSERT_EQUALS(round_up(15u, 16u), 16u);
		TS_ASSERT_EQUALS(round_up(20u, 32u), 32u);
		TS_ASSERT_EQUALS(round_up(29u, 32u), 32u);
		TS_ASSERT_EQUALS(round_up(0x1000u, 0x1000u), 0x1000u);
		TS_ASSERT_EQUALS(round_up(0x1001u, 0x1000u), 0x2000u);
		TS_ASSERT_EQUALS(round_up(0x1900u, 0x1000u), 0x2000u);
	}

	void test_round_down()
	{
		TS_ASSERT_EQUALS(round_down( 0u, 16u), 0u);
		TS_ASSERT_EQUALS(round_down( 4u, 16u), 0u);
		TS_ASSERT_EQUALS(round_down(15u, 16u), 0u);
		TS_ASSERT_EQUALS(round_down(20u, 16u), 16u);
		TS_ASSERT_EQUALS(round_down(29u, 16u), 16u);
		TS_ASSERT_EQUALS(round_down(0x1900u, 0x1000u), 0x1000u);
		TS_ASSERT_EQUALS(round_down(0x2001u, 0x2000u), 0x2000u);
	}

	// 16-bit saturating arithmetic
	void test_addusw()
	{
		TS_ASSERT_EQUALS(addusw(4u, 0x100u), 0x0104u);
		TS_ASSERT_EQUALS(addusw(0u, 0xFFFFu), 0xFFFFu);
		TS_ASSERT_EQUALS(addusw(0x8000u, 0x8000u), 0xFFFFu);
		TS_ASSERT_EQUALS(addusw(0xFFF0u, 0x0004u), 0xFFF4u);
		TS_ASSERT_EQUALS(addusw(0xFFFFu, 0xFFFFu), 0xFFFFu);
	}

	void test_subusw()
	{
		TS_ASSERT_EQUALS(subusw(4u, 0x100u), 0u);
		TS_ASSERT_EQUALS(subusw(100u, 90u), 10u);
		TS_ASSERT_EQUALS(subusw(0x8000u, 0x8000u), 0u);
		TS_ASSERT_EQUALS(subusw(0x0FFFu, 0xFFFFu), 0u);
		TS_ASSERT_EQUALS(subusw(0xFFFFu, 0x0FFFu), 0xF000u);
	}

	void test_movzx()
	{
		const u8 d1[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
		const u8 d2[] = { 0x43, 0x12, 0x23, 0xA4 };
		TS_ASSERT_EQUALS(movzx_64le(d1, 1), 0x01ull);
		TS_ASSERT_EQUALS(movzx_64le(d1, 2), 0x0201ull);
		TS_ASSERT_EQUALS(movzx_64le(d1, 8), 0x0807060504030201ull);
		TS_ASSERT_EQUALS(movzx_64le(d2, 4), 0xA4231243ull);
		TS_ASSERT_EQUALS(movzx_64le(d2+3, 1), 0xA4ull);
	}

	void test_movsx()
	{
		const u8 d1[] = { 0x09, 0xFE };
		const u8 d2[] = { 0xD9, 0x2C, 0xDD, 0x8F };
		const u8 d3[] = { 0x92, 0x26, 0x88, 0xF1, 0x35, 0xAC, 0x01, 0x83 };
		TS_ASSERT_EQUALS(movsx_64le(d1, 1), 0x09ull);
		TS_ASSERT_EQUALS(movsx_64le(d1, 2), 0xFFFFFFFFFFFFFE09ull);
		TS_ASSERT_EQUALS(movsx_64le(d2, 4), 0xFFFFFFFF8FDD2CD9ull);
		TS_ASSERT_EQUALS(movsx_64le(d3, 8), 0x8301AC35F1882692ull);
	}

	void test_hi_lo()
	{
		TS_ASSERT_EQUALS(u64_hi(0x0123456789ABCDEFull), 0x01234567u);
		TS_ASSERT_EQUALS(u64_hi(0x0000000100000002ull), 0x00000001u);

		TS_ASSERT_EQUALS(u64_lo(0x0123456789ABCDEFull), 0x89ABCDEFu);
		TS_ASSERT_EQUALS(u64_lo(0x0000000100000002ull), 0x00000002u);

		TS_ASSERT_EQUALS(u32_hi(0x01234567u), 0x0123u);
		TS_ASSERT_EQUALS(u32_hi(0x00000001u), 0x0000u);

		TS_ASSERT_EQUALS(u32_lo(0x01234567u), 0x4567u);
		TS_ASSERT_EQUALS(u32_lo(0x00000001u), 0x0001u);

		TS_ASSERT_EQUALS(u64_from_u32(0xFFFFFFFFu, 0x80000008u), 0xFFFFFFFF80000008ull);
		TS_ASSERT_EQUALS(u32_from_u16(0x8000u, 0xFFFFu), 0x8000FFFFu);
	}

	// fp_to_u?? already validate the result.

	void test_wildcard()
	{
		TS_ASSERT_EQUALS(match_wildcard("", ""), 1);
		TS_ASSERT_EQUALS(match_wildcard("a", 0), 1);	// NULL matches everything

		TS_ASSERT_EQUALS(match_wildcard("abc", "abc")     , 1);	// direct match
		TS_ASSERT_EQUALS(match_wildcard("abc", "???")     , 1);	// only ?
		TS_ASSERT_EQUALS(match_wildcard("abc", "*"  )     , 1);	// only *

		TS_ASSERT_EQUALS(match_wildcard("ab" , "a?" )     , 1);	// trailing ?
		TS_ASSERT_EQUALS(match_wildcard("abc", "a?c")     , 1);	// middle ?
		TS_ASSERT_EQUALS(match_wildcard("abc", "?bc")     , 1);	// leading ?

		TS_ASSERT_EQUALS(match_wildcard("abc", "a*" )     , 1);	// trailing *
		TS_ASSERT_EQUALS(match_wildcard("abcdef", "ab*ef"), 1);	// middle *
		TS_ASSERT_EQUALS(match_wildcard("abcdef", "*f"   ), 1);	// leading *

		TS_ASSERT_EQUALS(match_wildcard("abcdef", "a?cd*"), 1);	// ? and *
		TS_ASSERT_EQUALS(match_wildcard("abcdef", "a*d?f"), 1);	// * and ?
		TS_ASSERT_EQUALS(match_wildcard("abcdef", "a*d*" ), 1);	// multiple *

		// unicode test pasted from the above; keep in sync!

		TS_ASSERT_EQUALS(match_wildcardw(L"", L""), 1);
		TS_ASSERT_EQUALS(match_wildcardw(L"a", 0), 1);	// NULL matches everything

		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"abc")     , 1);	// direct match
		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"???")     , 1);	// only ?
		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"*"  )     , 1);	// only *

		TS_ASSERT_EQUALS(match_wildcardw(L"ab" , L"a?" )     , 1);	// trailing ?
		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"a?c")     , 1);	// middle ?
		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"?bc")     , 1);	// leading ?

		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"a*" )     , 1);	// trailing *
		TS_ASSERT_EQUALS(match_wildcardw(L"abcdef", L"ab*ef"), 1);	// middle *
		TS_ASSERT_EQUALS(match_wildcardw(L"abcdef", L"*f"   ), 1);	// leading *

		TS_ASSERT_EQUALS(match_wildcardw(L"abcdef", L"a?cd*"), 1);	// ? and *
		TS_ASSERT_EQUALS(match_wildcardw(L"abcdef", L"a*d?f"), 1);	// * and ?
		TS_ASSERT_EQUALS(match_wildcardw(L"abcdef", L"a*d*" ), 1);	// multiple *
	}

	void test_base32()
	{
		// compare against previous output (generated via this base32() call)
		const u8 in[] = { 0x12, 0x57, 0x85, 0xA2, 0xF9, 0x41, 0xCD, 0x57, 0xF3 };
		u8 out[20] = {0};
		base32(ARRAY_SIZE(in), in, out);
		const u8 correct_out[] = "CJLYLIXZIHGVP4C";
		TS_ASSERT_SAME_DATA(out, correct_out, ARRAY_SIZE(correct_out));
	}

	void test_rand()
	{
		// complain if huge interval or min > max
		debug_skip_next_err(ERR::INVALID_PARAM);
		TS_ASSERT_EQUALS(rand(1, 0), 0);
		debug_skip_next_err(ERR::INVALID_PARAM);
		TS_ASSERT_EQUALS(rand(2, ~0u), 0);

		// returned number must be in [min, max)
		for(int i = 0; i < 100; i++)
		{
			uint min = rand(), max = min+rand();
			uint x = rand(min, max);
			TS_ASSERT(min <= x && x < max);
		}

		// make sure both possible values are hit
		uint ones = 0, twos = 0;
		for(int i = 0; i < 100; i++)
		{
			uint x = rand(1, 3);
			// paranoia: don't use array (x might not be 1 or 2 - checked below)
			if(x == 1) ones++;
			if(x == 2) twos++;
		}
		TS_ASSERT_EQUALS(ones+twos, 100);
		TS_ASSERT(ones > 10 && twos > 10);
	}
};
