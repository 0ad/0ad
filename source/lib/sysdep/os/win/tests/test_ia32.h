/* Copyright (C) 2010 Wildfire Games.
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

#include "lib/sysdep/arch/x86_x64/x86_x64.h"

// note: ia32_i??_from_*, ia32_rint*, ia32_fm??f are all tested within
// sysdep to avoid test duplication (both the ia32 versions and
// the portable fallback must behave the same).

class TestIA32: public CxxTest::TestSuite
{
public:
	void test_rdtsc()
	{
		// must increase monotonously
		const u64 c1 = x86_x64::rdtsc();
		const u64 c2 = x86_x64::rdtsc();
		const u64 c3 = x86_x64::rdtsc();
		TS_ASSERT(c1 < c2 && c2 < c3);
	}

	void test_ia32_cap()
	{
		// make sure the really common/basic caps end up reported as true
		TS_ASSERT(x86_x64::Cap(x86_x64::CAP_FPU));
		TS_ASSERT(x86_x64::Cap(x86_x64::CAP_TSC));
		TS_ASSERT(x86_x64::Cap(x86_x64::CAP_MMX));
	}
};
