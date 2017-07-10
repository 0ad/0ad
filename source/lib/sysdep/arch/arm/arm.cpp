/* Copyright (C) 2012 Wildfire Games.
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
 * routines specific to ARM
 */

#include "precompiled.h"

#include "lib/sysdep/cpu.h"

intptr_t cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment)
{
	return __sync_fetch_and_add(location, increment);
}

bool cpu_CAS(volatile intptr_t* location, intptr_t expected, intptr_t newValue)
{
	return __sync_bool_compare_and_swap(location, expected, newValue);
}

bool cpu_CAS64(volatile i64* location, i64 expected, i64 newValue)
{
	return __sync_bool_compare_and_swap(location, expected, newValue);
}

const char* cpu_IdentifierString()
{
	return "unknown"; // TODO
}
