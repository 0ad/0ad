/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "lib/self_test.h"

#include "lib/bits.h"

//#define EQUALS(actual, expected) ENSURE((actual) == (expected))
#define EQUALS TS_ASSERT_EQUALS

class TestBits : public CxxTest::TestSuite
{
public:
	void test_Bit()
	{
		EQUALS(Bit<unsigned>(0), 1u);
		EQUALS(Bit<unsigned>(8), 0x100u);
		EQUALS(Bit<u32>(31), u32(0x80000000ul));
		EQUALS(Bit<u64>(1), u64(2));
		EQUALS(Bit<u64>(32), u64(0x100000000ull));
		EQUALS(Bit<u64>(63), u64(0x8000000000000000ull));
	}

	void test_IsBitSet()
	{
		EQUALS(IsBitSet(0u, 1), false);
		EQUALS(IsBitSet(1u, 1), false);
		EQUALS(IsBitSet(2u, 1), true);
		EQUALS(IsBitSet<u32>(0xFFFFFFFFul, 0), true);
		EQUALS(IsBitSet<u32>(0xFFFFFFFFul, 31), true);
		EQUALS(IsBitSet<u64>(0xFFFFFFFFFFFFFFFFull, 0), true);
		EQUALS(IsBitSet<u64>(0xFFFFFFFFFFFFFFFFull, 31), true);
		EQUALS(IsBitSet<u64>(0xFFFFFFFFFFFFFFFFull, 32), true);
		EQUALS(IsBitSet<u64>(0xFFFFFFFFFFFFFFFFull, 63), true);
	}

	void test_bit_mask()
	{
		EQUALS(bit_mask<u16>(0), 0);
		EQUALS(bit_mask<u16>(2), 0x3);
		EQUALS(bit_mask<u16>(16), 0xFFFF);
		EQUALS(bit_mask<u32>(0), 0u);
		EQUALS(bit_mask<u32>(2), 0x3u);
		EQUALS(bit_mask<u32>(32), 0xFFFFFFFFul);
		EQUALS(bit_mask<u64>(0), 0u);
		EQUALS(bit_mask<u64>(2), 0x3u);
		EQUALS(bit_mask<u64>(32), 0xFFFFFFFFull);
		EQUALS(bit_mask<u64>(64), 0xFFFFFFFFFFFFFFFFull);
	}

	void test_bits()
	{
		EQUALS(bits<u16>(0xFFFF, 0, 15), 0xFFFF);
		EQUALS(bits<u16>(0xFFFF, 0, 7), 0xFF);
		EQUALS(bits<u16>(0xFFFF, 8, 15), 0xFF);
		EQUALS(bits<u16>(0xFFFF, 14, 15), 0x3);
		EQUALS(bits<u16>(0xAA55, 4, 11), 0xA5);
		EQUALS(bits<u16>(0xAA55, 14, 15), 0x2);
		EQUALS(bits<u32>(0ul, 0, 31), 0ul);
		EQUALS(bits<u32>(0xFFFFFFFFul, 0, 31), 0xFFFFFFFFul);
		EQUALS(bits<u64>(0ull, 0, 63), 0ull);
		EQUALS(bits<u64>(0xFFFFFFFFull, 0, 31), 0xFFFFFFFFull);
		EQUALS(bits<u64>(0x0000FFFFFFFF0000ull, 16, 47), 0xFFFFFFFFull);
		EQUALS(bits<u64>(0xFFFFFFFFFFFFFFFFull, 0, 63), 0xFFFFFFFFFFFFFFFFull);
		EQUALS(bits<u64>(0xA5A5A5A5A5A5A5A5ull, 32, 63), 0xA5A5A5A5ull);
	}

	void test_PopulationCount()
	{
		EQUALS(PopulationCount<u8>(0), 0u);
		EQUALS(PopulationCount<u8>(4), 1u);
		EQUALS(PopulationCount<u8>(0x28), 2u);
		EQUALS(PopulationCount<u8>(0xFF), 8u);
		EQUALS(PopulationCount<u32>(0x0ul), 0u);
		EQUALS(PopulationCount<u32>(0x8ul), 1u);
		EQUALS(PopulationCount<u32>(0xFFFFul), 16u);
		EQUALS(PopulationCount<u32>(0xFFFFFFFFul), 32u);
		EQUALS(PopulationCount<u64>(0x0ull), 0u);
		EQUALS(PopulationCount<u64>(0x10ull), 1u);
		EQUALS(PopulationCount<u64>(0xFFFFull), 16u);
		EQUALS(PopulationCount<u64>(0xFFFFFFFFull), 32u);
		EQUALS(PopulationCount<u64>(0xFFFFFFFFFFFFFFFEull), 63u);
		EQUALS(PopulationCount<u64>(0xFFFFFFFFFFFFFFFFull), 64u);
	}

	void test_is_pow2()
	{
		EQUALS(is_pow2(0u), false);
		EQUALS(is_pow2(~0u), false);
		EQUALS(is_pow2(0x80000001), false);
		EQUALS(is_pow2(1), true);
		EQUALS(is_pow2(1u << 31), true);
	}

	void test_ceil_log2()
	{
		EQUALS(ceil_log2(3u), 2u);
		EQUALS(ceil_log2(0xffffffffu), 32u);
		EQUALS(ceil_log2(1u), 0u);
		EQUALS(ceil_log2(256u), 8u);
		EQUALS(ceil_log2(0x80000000u), 31u);
	}

	void test_floor_log2()
	{
		EQUALS(floor_log2(1.f), 0);
		EQUALS(floor_log2(3.f), 1);
		EQUALS(floor_log2(256.f), 8);
	}

	void test_round_up_to_pow2()
	{
		EQUALS(round_up_to_pow2(0u), 1u);
		EQUALS(round_up_to_pow2(1u), 1u);
		EQUALS(round_up_to_pow2(127u), 128u);
		EQUALS(round_up_to_pow2(128u), 128u);
		EQUALS(round_up_to_pow2(129u), 256u);
	}

	void test_round_down_to_pow2()
	{
		EQUALS(round_down_to_pow2(1u), 1u);
		EQUALS(round_down_to_pow2(127u), 64u);
		EQUALS(round_down_to_pow2(128u), 128u);
		EQUALS(round_down_to_pow2(129u), 128u);
	}

	void test_round_up()
	{
		EQUALS(round_up( 0u, 16u), 0u);
		EQUALS(round_up( 4u, 16u), 16u);
		EQUALS(round_up(15u, 16u), 16u);
		EQUALS(round_up(20u, 32u), 32u);
		EQUALS(round_up(29u, 32u), 32u);
		EQUALS(round_up(0x1000u, 0x1000u), 0x1000u);
		EQUALS(round_up(0x1001u, 0x1000u), 0x2000u);
		EQUALS(round_up(0x1900u, 0x1000u), 0x2000u);
	}

	void test_round_down()
	{
		EQUALS(round_down( 0u, 16u), 0u);
		EQUALS(round_down( 4u, 16u), 0u);
		EQUALS(round_down(15u, 16u), 0u);
		EQUALS(round_down(20u, 16u), 16u);
		EQUALS(round_down(29u, 16u), 16u);
		EQUALS(round_down(0x1900u, 0x1000u), 0x1000u);
		EQUALS(round_down(0x2001u, 0x2000u), 0x2000u);
	}
};
