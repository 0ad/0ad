/* Copyright (C) 2010 Wildfire Games.
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
 * model-specific registers
 */

#ifndef INCLUDED_X86_X64_MSR
#define INCLUDED_X86_X64_MSR

namespace MSR {

enum ModelSpecificRegisters
{
	// architectural (will not change on future processors)
	IA32_MISC_ENABLE            = 0x1A0,
	IA32_ENERGY_PERF_BIAS       = 0x1B0,	// requires HasEnergyPerfBias

	// PMU v1
	IA32_PMC0                   = 0x0C1,
	IA32_PERFEVTSEL0            = 0x186,

	// PMU v2
	IA32_PERF_GLOBAL_STATUS     = 0x38E,
	IA32_PERF_GLOBAL_CTRL       = 0x38F,
	IA32_PERF_GLOBAL_OVF_CTRL   = 0x390,

	// Nehalem and later
	PLATFORM_INFO               = 0x0CE,	// requires HasPlatformInfo

	// Nehalem, Westmere (requires HasUncore)
	UNCORE_PERF_GLOBAL_CTRL     = 0x391,
	UNCORE_PERF_GLOBAL_STATUS   = 0x392,
	UNCORE_PERF_GLOBAL_OVF_CTRL = 0x393,
	UNCORE_PMC0                 = 0x3B0,
	UNCORE_PERFEVTSEL0          = 0x3C0
};

LIB_API bool IsAccessible();

LIB_API bool HasEnergyPerfBias();
LIB_API bool HasPlatformInfo();
LIB_API bool HasUncore();

LIB_API u64 Read(u64 reg);
LIB_API void Write(u64 reg, u64 value);

}	// namespace MSR

#endif	// #ifndef INCLUDED_X86_X64_MSR
