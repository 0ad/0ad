// helpers for built-in self tests
//
// Copyright (c) 2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include "self_test.h"

// checked by debug_assert_failed; disables asserts if true (see above).
// set/cleared by run_self_test.
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
