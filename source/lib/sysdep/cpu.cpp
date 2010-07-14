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
 * CPU and memory detection.
 */

#include "precompiled.h"
#include "lib/sysdep/cpu.h"

ERROR_ASSOCIATE(ERR::CPU_FEATURE_MISSING, L"This CPU doesn't support a required feature", -1);
ERROR_ASSOCIATE(ERR::CPU_UNKNOWN_OPCODE, L"Disassembly failed", -1);
ERROR_ASSOCIATE(ERR::CPU_UNKNOWN_VENDOR, L"CPU vendor unknown", -1);

void cpu_TestAtomicAdd()
{
	volatile intptr_t i1 = 1;
	intptr_t prev = cpu_AtomicAdd(&i1, 1);
	debug_assert(prev == 1);
	debug_assert(i1 == 2);
}
