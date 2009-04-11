#include "lib/self_test.h"

// usually defined by main.cpp, used by engine's scripting/ScriptGlue.cpp,
// must be included here to placate linker.
void kill_mainloop()
{
}

// just so that cxxtestgen doesn't complain "No tests defined"
class TestDummy : public CxxTest::TestSuite 
{
public:
	void test_dummy()
	{
	}
};
