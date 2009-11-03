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

/*
 * helpers for built-in self tests
 */

#include "precompiled.h"

#if 0

#include "self_test.h"
#include "timer.h"

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
