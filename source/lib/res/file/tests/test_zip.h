#include "lib/self_test.h"

#include <time.h>

#include "lib/lib.h"
#include "lib/res/file/zip.h"

class TestZip : public CxxTest::TestSuite 
{
public:
	void test_fat_timedate_conversion()
	{
		// note: FAT time stores second/2, which means converting may
		// end up off by 1 second.

		time_t t, converted_t;

		t = time(0);
		converted_t = time_t_from_FAT(FAT_from_time_t(t));
		TS_ASSERT_DELTA(t, converted_t, 2);

		t++;
		converted_t = time_t_from_FAT(FAT_from_time_t(t));
		TS_ASSERT_DELTA(t, converted_t, 2);
	}
};
