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

static const StatusDefinition cpuStatusDefinitions[] = {
	{ ERR::CPU_FEATURE_MISSING, L"This CPU doesn't support a required feature" },
	{ ERR::CPU_UNKNOWN_OPCODE, L"Disassembly failed" },
	{ ERR::CPU_UNKNOWN_VENDOR, L"CPU vendor unknown" }
};
STATUS_ADD_DEFINITIONS(cpuStatusDefinitions);


// ensure the actual pointer size matches expectations on the most common
// architectures (IA-32 and AMD64) - just in case the predefined macros
// are wrong or misleading.
#if ARCH_IA32
cassert(sizeof(void*) == 4);
#elif ARCH_AMD64
cassert(sizeof(void*) == 8);
cassert(sizeof(i64) == sizeof(intptr_t));
#endif
cassert(sizeof(void*) == sizeof(intptr_t));



static void TestCAS64()
{
	volatile i64 var = 1;
	cpu_CAS64(&var, 1ull, 2ull);
	ENSURE(var == 2ull);
}

static void TestAtomicAdd()
{
	volatile intptr_t i1 = 1;
	intptr_t prev = cpu_AtomicAdd(&i1, 1);
	ENSURE(prev == 1);
	ENSURE(i1 == 2);
}

void cpu_Test()
{
	TestCAS64();
	TestAtomicAdd();
}
