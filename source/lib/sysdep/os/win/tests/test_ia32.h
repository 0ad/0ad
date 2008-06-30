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
		const u64 c1 = x86_x64_rdtsc();
		const u64 c2 = x86_x64_rdtsc();
		const u64 c3 = x86_x64_rdtsc();
		TS_ASSERT(c1 < c2 && c2 < c3);
	}

	void test_ia32_cap()
	{
		// make sure the really common/basic caps end up reported as true
		TS_ASSERT(x86_x64_cap(X86_X64_CAP_FPU));
		TS_ASSERT(x86_x64_cap(X86_X64_CAP_TSC));
		TS_ASSERT(x86_x64_cap(X86_X64_CAP_MMX));
	}
};
