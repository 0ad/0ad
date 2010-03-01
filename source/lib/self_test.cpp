/* Copyright (c) 2010 Wildfire Games
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

/*
 * helpers for built-in self tests
 */

#include "precompiled.h"

#if 0

#include "lib/self_test.h"
#include "lib/timer.h"

// checked by debug_OnAssertionFailure; disables asserts if true (see above).
// set/cleared by self_test_run.
bool self_test_active = false;

// trampoline that sets self_test_active and returns a dummy value;
// used by SELF_TEST_RUN.
int self_test_run(void (*func)())
{
	self_test_active = true;
	func();
	self_test_active = false;
	return 0;	// assigned to dummy at file scope
}


static const SelfTestRecord* registered_tests;

int self_test_register(SelfTestRecord* r)
{
	// SELF_TEST_REGISTER has already initialized r->func.
	r->next = registered_tests;
	registered_tests = r;
	return 0;	// assigned to dummy at file scope
}


void self_test_run_all()
{
	debug_printf(L"SELF TESTS:\n");
	const double t0 = timer_Time();

	// someone somewhere may want to run self-tests twice (e.g. to help
	// track down memory corruption), so don't destroy the list while
	// iterating over it.
	const SelfTestRecord* r = registered_tests;
	while(r)
	{
		self_test_run(r->func);
		r = r->next;
	}

	const double dt = timer_Time() - t0;
	debug_printf(L"-- done (elapsed time %.0f ms)\n", dt*1e3);
}

#endif
