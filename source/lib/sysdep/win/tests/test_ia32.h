#include "lib/self_test.h"

#include "lib/lib.h"
#include "lib/sysdep/ia32/ia32.h"

// note: ia32_i??_from_*, ia32_rint*, ia32_fm??f are all tested within
// sysdep to avoid test duplication (both the ia32 versions and
// the portable fallback must behave the same).

class TestIA32: public CxxTest::TestSuite 
{
public:
	void test_rdtsc()
	{
		// must increase monotonously
		const u64 c1 = ia32_rdtsc();
		const u64 c2 = ia32_rdtsc();
		const u64 c3 = ia32_rdtsc();
		TS_ASSERT(c1 < c2 && c2 < c3);
	}

	void test_ia32_cap()
	{
		ia32_Init();

		// make sure the really common/basic caps end up reported as true
		TS_ASSERT(ia32_cap(IA32_CAP_FPU));
		TS_ASSERT(ia32_cap(IA32_CAP_TSC));
		TS_ASSERT(ia32_cap(IA32_CAP_MMX));
	}
};
