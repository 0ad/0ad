#include "precompiled.h"

#include "self_test.h"

// if true, debug_assert does nothing.
bool self_test_active = false;

// trampoline that sets self_test_active and returns a dummy value;
// used by RUN_SELF_TEST.
int run_self_test(void(*test_func)())
{
	self_test_active = true;
	test_func();
	self_test_active = false;
	return 0;
}