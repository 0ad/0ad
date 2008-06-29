#include "lib/self_test.h"

#include "graphics/Color.h"

class TestColor : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		ColorActivateFastImpl();
	}

	void test_Color4ub()
	{
#define T(r, g, b, ub) TS_ASSERT_EQUALS(ub | 0xff000000, ConvertRGBColorTo4ub(RGBColor(r,g,b)))
		T(0, 0, 0, 0x000000);
		T(1, 0, 0, 0x0000ff);
		T(0, 1, 0, 0x00ff00);
		T(0, 0, 1, 0xff0000);
		T(1, 1, 1, 0xffffff);
#undef T
	}
};
