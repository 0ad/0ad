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

#include "lib/fnv_hash.h"

class TestFnvHash : public CxxTest::TestSuite 
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
};
