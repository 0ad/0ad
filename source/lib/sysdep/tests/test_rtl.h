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

#include "lib/sysdep/rtl.h"

class Test_rtl : public CxxTest::TestSuite
{
	void _test_AllocateAligned_helper(size_t size, size_t align)
	{
		void* p = rtl_AllocateAligned(size, align);
		TS_ASSERT(p != NULL);
		TS_ASSERT_EQUALS((intptr_t)p % align, 0u);
		memset(p, 0x42, size);
		rtl_FreeAligned(p);
	}
public:
	void test_AllocateAligned()
	{
		for (size_t s = 0; s < 64; ++s)
		{
			_test_AllocateAligned_helper(s, 8);
			_test_AllocateAligned_helper(s, 16);
			_test_AllocateAligned_helper(s, 64);
			_test_AllocateAligned_helper(s, 1024);
			_test_AllocateAligned_helper(s, 65536);
		}
	}

	void test_FreeAligned_null()
	{
		rtl_FreeAligned(NULL);
	}
};
