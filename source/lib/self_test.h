
#ifndef SELF_TEST_ENABLED
#define SELF_TEST_ENABLED 1
#endif

// each test case should use this to verify conditions.
// note: could also stringize condition and display that, but it'd require
// macro magic (stringize+prepend L) and we already get file+line.
#define TEST(condition) STMT(\
	if(!(condition))\
		DISPLAY_ERROR(L"Self-test failed");\
)

extern bool self_test_active;

extern int run_self_test(void(*test_func)());
#define RUN_SELF_TEST static int dummy = run_self_test(self_test)
