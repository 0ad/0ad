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
 * CPU-specific routines common to 32 and 64-bit x86
 */

#include "precompiled.h"
#include "lib/sysdep/arch/x86_x64/x86_x64.h"

#include <cstring>
#include <cstdio>
#include <vector>
#include <set>
#include <algorithm>

#include "lib/posix/posix.h"	// pthread
#include "lib/bits.h"
#include "lib/timer.h"
#include "lib/module_init.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/os_cpu.h"

#if MSC_VERSION
# include <intrin.h>	// __rdtsc
#endif

#define CPUID_INTRINSIC 0
#if MSC_VERSION >= 1500	// __cpuidex available (allows setting ecx beforehand)
# undef CPUID_INTRINSIC
# define CPUID_INTRINSIC 1
#elif GCC_VERSION
// (no additional includes needed)
#else
# if ARCH_AMD64
#  include "lib/sysdep/arch/amd64/amd64_asm.h"
# else
#  include "lib/sysdep/arch/ia32/ia32_asm.h"
# endif
#endif


// some of this module's functions are frequently called but require
// non-trivial initialization, so caching is helpful. isInitialized
// flags aren't thread-safe, so we use ModuleInit. calling it from
// every function is a bit wasteful, but it is convenient to avoid
// requiring users to pass around a global state object.
// one big Init() would be prone to deadlock if its subroutines also
// call a public function (that re-enters ModuleInit), so each
// function gets its own initState.


//-----------------------------------------------------------------------------
// CPUID

static void cpuid(x86_x64_CpuidRegs* regs)
{
#if CPUID_INTRINSIC
	cassert(sizeof(regs->eax) == sizeof(int));
	cassert(sizeof(*regs) == 4*sizeof(int));
	__cpuidex((int*)regs, regs->eax, regs->ecx);
#elif GCC_VERSION
	__asm__ __volatile__ ("cpuid": "=a" (regs->eax), "=b" (regs->ebx), "=c" (regs->ecx), "=d" (regs->edx) : "a" (regs->eax), "c" (regs->ecx));
#else
# if ARCH_AMD64
	amd64_asm_cpuid(regs);
# else
	ia32_asm_cpuid(regs);
# endif
#endif
}

static u32 cpuid_maxFunction;
static u32 cpuid_maxExtendedFunction;

static LibError InitCpuid()
{
	x86_x64_CpuidRegs regs = { 0 };

	regs.eax = 0;
	cpuid(&regs);
	cpuid_maxFunction = regs.eax;

	regs.eax = 0x80000000;
	cpuid(&regs);
	cpuid_maxExtendedFunction = regs.eax;

	return INFO::OK;
}

bool x86_x64_cpuid(x86_x64_CpuidRegs* regs)
{
	static ModuleInitState initState;
	ModuleInit(&initState, InitCpuid);

	const u32 function = regs->eax;
	if(function > cpuid_maxExtendedFunction)
		return false;
	if(function < 0x80000000 && function > cpuid_maxFunction)
		return false;

	cpuid(regs);
	return true;
}


//-----------------------------------------------------------------------------
// capability bits

// treated as 128 bit field; order: std ecx, std edx, ext ecx, ext edx
// keep in sync with enum x86_x64_Cap!
static u32 caps[4];

static ModuleInitState capsInitState;

static LibError InitCaps()
{
	x86_x64_CpuidRegs regs = { 0 };
	regs.eax = 1;
	if(x86_x64_cpuid(&regs))
	{
		caps[0] = regs.ecx;
		caps[1] = regs.edx;
	}
	regs.eax = 0x80000001;
	if(x86_x64_cpuid(&regs))
	{
		caps[2] = regs.ecx;
		caps[3] = regs.edx;
	}

	return INFO::OK;
}

bool x86_x64_cap(x86_x64_Cap cap)
{
	ModuleInit(&capsInitState, InitCaps);

	const size_t index = cap >> 5;
	const size_t bit = cap & 0x1F;
	if(index >= ARRAY_SIZE(caps))
	{
		DEBUG_WARN_ERR(ERR::INVALID_PARAM);
		return false;
	}
	return IsBitSet(caps[index], bit);
}

void x86_x64_caps(u32* d0, u32* d1, u32* d2, u32* d3)
{
	ModuleInit(&capsInitState, InitCaps);

	*d0 = caps[0];
	*d1 = caps[1];
	*d2 = caps[2];
	*d3 = caps[3];
}


//-----------------------------------------------------------------------------
// vendor

static x86_x64_Vendors vendor;

static LibError InitVendor()
{
	x86_x64_CpuidRegs regs = { 0 };
	regs.eax = 0;
	if(!x86_x64_cpuid(&regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);

	// copy regs to string
	// note: 'strange' ebx,edx,ecx reg order is due to ModR/M encoding order.
	char vendorString[13];
	memcpy(&vendorString[0], &regs.ebx, 4);
	memcpy(&vendorString[4], &regs.edx, 4);
	memcpy(&vendorString[8], &regs.ecx, 4);
	vendorString[12] = '\0';	// 0-terminate

	if(!strcmp(vendorString, "AuthenticAMD"))
		vendor = X86_X64_VENDOR_AMD;
	else if(!strcmp(vendorString, "GenuineIntel"))
		vendor = X86_X64_VENDOR_INTEL;
	else
	{
		DEBUG_WARN_ERR(ERR::CPU_UNKNOWN_VENDOR);
		vendor = X86_X64_VENDOR_UNKNOWN;
	}

	return INFO::OK;
}

x86_x64_Vendors x86_x64_Vendor()
{
	static ModuleInitState initState;
	ModuleInit(&initState, InitVendor);
	return vendor;
}


//-----------------------------------------------------------------------------
// signature

static size_t model;
static size_t family;
static ModuleInitState signatureInitState;

static LibError InitSignature()
{
	x86_x64_CpuidRegs regs = { 0 };
	regs.eax = 1;
	if(!x86_x64_cpuid(&regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
	model = bits(regs.eax, 4, 7);
	family = bits(regs.eax, 8, 11);
	const size_t extendedModel = bits(regs.eax, 16, 19);
	const size_t extendedFamily = bits(regs.eax, 20, 27);
	if(family == 0xF)
		family += extendedFamily;
	if(family == 0xF || (x86_x64_Vendor() == X86_X64_VENDOR_INTEL && family == 6))
		model += extendedModel << 4;
	return INFO::OK;
}

size_t x86_x64_Model()
{
	ModuleInit(&signatureInitState, InitSignature);
	return model;
}

size_t x86_x64_Family()
{
	ModuleInit(&signatureInitState, InitSignature);
	return family;
}


//-----------------------------------------------------------------------------
// cache

static const size_t maxCacheLevels = 3;
static x86_x64_Cache cacheStorage[maxCacheLevels*2];
static x86_x64_Caches dcaches = { 0, cacheStorage };
static x86_x64_Caches icaches = { 0, cacheStorage+maxCacheLevels };

static const size_t maxTLBLevels = 15;
static x86_x64_TLB tlbStorage[maxTLBLevels*2];
static x86_x64_TLBs dtlbs = { 0, tlbStorage };
static x86_x64_TLBs itlbs = { 0, tlbStorage+maxTLBLevels };

static bool IsData(x86_x64_CacheType type)
{
	return (type == X86_X64_CACHE_TYPE_DATA || type == X86_X64_CACHE_TYPE_UNIFIED);
}

static bool IsInstruction(x86_x64_CacheType type)
{
	return (type == X86_X64_CACHE_TYPE_INSTRUCTION || type == X86_X64_CACHE_TYPE_UNIFIED);
}

static void AddTLB(const x86_x64_TLB& tlb)
{
	if(IsInstruction(tlb.type))
	{
		if(itlbs.numLevels < maxTLBLevels)
			itlbs.levels[itlbs.numLevels++] = tlb;
		else
			debug_assert(0);
	}
	if(IsData(tlb.type))
	{
		if(dtlbs.numLevels < maxTLBLevels)
			dtlbs.levels[dtlbs.numLevels++] = tlb;
		else
			debug_assert(0);
	}

	// large page TLBs have N 2M entries or N/2 4M entries; we generate a
	// second set of parameters for the latter from the former.
	if(tlb.pageSize == 2*MiB)
	{
		x86_x64_TLB tlb4M = tlb;
		tlb4M.pageSize = 4*MiB;
		tlb4M.entries  = tlb.entries/2;
		AddTLB(tlb4M);
	}
}

namespace AMD
{

static x86_x64_Cache L1Cache(u32 reg, x86_x64_CacheType type)
{
	x86_x64_Cache cache;
	cache.type          = type;
	cache.level         = 1;
	cache.associativity = bits(reg, 16, 23);
	cache.lineSize      = bits(reg,  0,  7);
	cache.sharedBy      = 1;
	cache.totalSize     = bits(reg, 24, 31)*KiB;
	return cache;
}

// applies to L2, L3 and TLB2
static const size_t associativities[16] =
{
	0, 1, 2, 0, 4, 0, 8, 0,
	16, 0, 32, 48, 64, 96, 128, x86_x64_fullyAssociative
};

static x86_x64_Cache L2Cache(u32 reg, x86_x64_CacheType type)
{
	x86_x64_Cache cache;
	const size_t associativityIndex = bits(reg, 12, 15);
	if(associativityIndex == 0)	// disabled
	{
		cache.type = X86_X64_CACHE_TYPE_NULL;
		cache.associativity = 0;
	}
	else
	{
		cache.type = type;
		cache.associativity = associativities[associativityIndex];
		debug_assert(cache.associativity != 0);	// else: encoding is "reserved"
	}
	cache.level = 2;
	cache.lineSize  = bits(reg,  0,  7);
	cache.sharedBy  = 1;
	cache.totalSize = bits(reg, 16, 31)*KiB;
	return cache;
}

// (same as L2 except for the totalSize encoding)
static x86_x64_Cache L3Cache(u32 reg, x86_x64_CacheType type)
{
	x86_x64_Cache cache = L2Cache(reg, type);
	cache.level = 3;
	cache.totalSize = bits(reg, 18, 31)*512*KiB;	// (rounded down)
	return cache;
}

static x86_x64_TLB TLB1(u32 reg, size_t bitOffset, size_t pageSize, x86_x64_CacheType type)
{
	x86_x64_TLB tlb;
	tlb.type          = type;
	tlb.level         = 1;
	tlb.associativity = bits(reg, bitOffset+8, bitOffset+15);
	tlb.pageSize      = pageSize;
	tlb.entries       = bits(reg, bitOffset, bitOffset+7);
	return tlb;
}

static void AddTLB1(const x86_x64_CpuidRegs& regs)
{
	AddTLB(TLB1(regs.eax,  0, 2*MiB, X86_X64_CACHE_TYPE_INSTRUCTION));
	AddTLB(TLB1(regs.eax, 16, 2*MiB, X86_X64_CACHE_TYPE_DATA));
	AddTLB(TLB1(regs.ebx,  0, 4*KiB, X86_X64_CACHE_TYPE_INSTRUCTION));
	AddTLB(TLB1(regs.ebx, 16, 4*KiB, X86_X64_CACHE_TYPE_DATA));
}

static x86_x64_TLB TLB2(u32 reg, size_t bitOffset, size_t pageSize, x86_x64_CacheType type)
{
	x86_x64_TLB tlb;
	const size_t associativityIndex = bits(reg, bitOffset+12, bitOffset+15);
	if(associativityIndex == 0)	// disabled
	{
		tlb.type = X86_X64_CACHE_TYPE_NULL;
		tlb.associativity = 0;
	}
	else
	{
		tlb.type = type;
		tlb.associativity = associativities[associativityIndex];
	}
	tlb.level    = 2;
	tlb.pageSize = pageSize;
	tlb.entries  = bits(reg, bitOffset, bitOffset+11);
	return tlb;
}

static void AddTLB2Pair(u32 reg, size_t pageSize)
{
	x86_x64_CacheType type = X86_X64_CACHE_TYPE_UNIFIED;
	if(bits(reg, 16, 31) != 0)	// not unified
	{
		AddTLB(TLB2(reg, 16, pageSize, X86_X64_CACHE_TYPE_DATA));
		type = X86_X64_CACHE_TYPE_INSTRUCTION;
	}
	AddTLB(TLB2(reg, 0, pageSize, type));
}

// AMD reports maxCpuidIdFunction > 4 but consider functions 2..4 to be
// "reserved". cache characteristics are returned via ext. functions.
static void DetectCacheAndTLB()
{
	x86_x64_CpuidRegs regs = { 0 };

	regs.eax = 0x80000005;
	if(x86_x64_cpuid(&regs))
	{
		AddTLB1(regs);

		dcaches.numLevels = icaches.numLevels = 1;
		dcaches.levels[0] = L1Cache(regs.ecx, X86_X64_CACHE_TYPE_DATA);
		icaches.levels[0] = L1Cache(regs.edx, X86_X64_CACHE_TYPE_INSTRUCTION);
	}

	regs.eax = 0x80000006;
	if(x86_x64_cpuid(&regs))
	{
		AddTLB2Pair(regs.eax, 2*MiB);
		AddTLB2Pair(regs.ebx, 4*KiB);

		icaches.numLevels = dcaches.numLevels = 2;
		icaches.levels[1] = dcaches.levels[1] = L2Cache(regs.ecx, X86_X64_CACHE_TYPE_UNIFIED);

		icaches.numLevels = dcaches.numLevels = 3;
		icaches.levels[2] = dcaches.levels[2] = L3Cache(regs.edx, X86_X64_CACHE_TYPE_UNIFIED);
	}
}

}	// namespace AMD

// note: CPUID 8000_0006 also returns L2 size, associativity, and
// line size, but I don't see any advantages vs. CPUID 4.

static void DetectCache_CPUID4()
{
	// note: ordering is undefined (see Intel AP-485)
	for(u32 count = 0; ; count++)
	{
		x86_x64_CpuidRegs regs = { 0 };
		regs.eax = 4;
		regs.ecx = count;
		if(!x86_x64_cpuid(&regs))
		{
			debug_assert(0);
			break;
		}

		const x86_x64_CacheType type = (x86_x64_CacheType)bits(regs.eax, 0, 4);
		const size_t level = (size_t)bits(regs.eax, 5, 7);
		if(type == X86_X64_CACHE_TYPE_NULL)	// no more remaining
			break;

		x86_x64_Cache cache;
		cache.type          = type;
		cache.level         = level;
		cache.associativity = (size_t)bits(regs.ebx, 22, 31)+1;
		cache.lineSize      = (size_t)bits(regs.ebx,  0, 11)+1;	// (yes, this also uses +1 encoding)
		cache.sharedBy      = (size_t)bits(regs.eax, 14, 25)+1;
		{
			const size_t partitions = (size_t)bits(regs.ebx, 12, 21)+1;
			const size_t sets = (size_t)bits(regs.ecx, 0, 31)+1;
			cache.totalSize = cache.associativity * partitions * cache.lineSize * sets;
		}

		if(IsInstruction(type))
		{
			icaches.numLevels = std::max(icaches.numLevels, level);
			icaches.levels[level-1] = cache;
		}
		if(IsData(type))
		{
			dcaches.numLevels = std::max(dcaches.numLevels, level);
			dcaches.levels[level-1] = cache;
		}
	}
}

static void ExtractDescriptors(u32 reg, std::vector<u8>& descriptors)
{
	if(IsBitSet(reg, 31))	// register contents are reserved
		return;
	for(int pos = 24; pos >= 0; pos -= 8)
	{
		const u8 descriptor = (u8)bits(reg, pos, pos+7);
		if(descriptor != 0)
			descriptors.push_back(descriptor);
	}
}

// note: the following cannot reside in DecodeDescriptor because
// ARRAY_SIZE's template argument must not reference a local type.

enum Flags
{
	// type (unified := neither bit set)
	I   = 0x01,
	D   = 0x02,

	// level
	L2  = 0x04,

	// size
	S4K = 0x08,
	S4M = 0x10,
	S2M = 0x20
};

struct Properties
{
	int flags;	// pageSize, type, level
	u8 descriptor;
	u8 associativity;
	u16 entries;
};

static const u8 F = x86_x64_fullyAssociative;

#define PROPERTIES(descriptor, flags, assoc, entries) { flags, descriptor, assoc, entries }

// references: [accessed 2009-01-05]
// AP485 http://download.intel.com/design/processor/applnots/241618033.pdf
// sandp http://www.sandpile.org/ia32/cpuid.htm
// opsol http://src.opensolaris.org/source/xref/onnv/onnv-gate/usr/src/uts/i86pc/os/cpuid.c
static const Properties propertyTable[] =
{
	PROPERTIES(0x01, I|S4K, 4,  32),
	PROPERTIES(0x02, I|S4M, F,   2),
	PROPERTIES(0x03, D|S4K, 4,  64),
	PROPERTIES(0x04, D|S4M, 4,   8),
	PROPERTIES(0x05, D|S4M, 4,  32),
	PROPERTIES(0x0B, I|S4M, 4,   4),
	PROPERTIES(0x4F, I|S4K, F,  32),	// sandp: unknown assoc, opsol: full, AP485: unmentioned
	PROPERTIES(0x50, I|S4K, F,  64),
	PROPERTIES(0x50, I|S4M, F,  64),
	PROPERTIES(0x50, I|S2M, F,  64),
	PROPERTIES(0x51, I|S4K, F, 128),
	PROPERTIES(0x51, I|S4M, F, 128),
	PROPERTIES(0x51, I|S2M, F, 128),
	PROPERTIES(0x52, I|S4K, F, 256),
	PROPERTIES(0x52, I|S4M, F, 256),
	PROPERTIES(0x52, I|S2M, F, 256),
	PROPERTIES(0x55, I|S4M, F,   7),
	PROPERTIES(0x55, I|S2M, F,   7),
	PROPERTIES(0x56, D|S4M, 4,  16),
	PROPERTIES(0x57, D|S4K, 4,  16),
	PROPERTIES(0x59, D|S4K, F,  16),
	PROPERTIES(0x5A, D|S4M, 4,  32),
	PROPERTIES(0x5A, D|S2M, 4,  32),
	PROPERTIES(0x5B, D|S4K, F,  64),
	PROPERTIES(0x5B, D|S4M, F,  64),
	PROPERTIES(0x5C, D|S4K, F, 128),
	PROPERTIES(0x5C, D|S4M, F, 128),
	PROPERTIES(0x5D, D|S4K, F, 256),
	PROPERTIES(0x5D, D|S4M, F, 256),
	PROPERTIES(0xB0, I|S4K, 4, 128),
	PROPERTIES(0xB1, I|S2M, 4,   8),
	PROPERTIES(0xB1, I|S4M, 4,   4),
	PROPERTIES(0xB2, I|S4K, 4,  64),
	PROPERTIES(0xB3, D|S4K, 4, 128),
	PROPERTIES(0xB3, D|S4M, 4, 128),
	PROPERTIES(0xB4, D|S4K, 4, 256),
	PROPERTIES(0xB4, D|S4M, 4, 256),
	PROPERTIES(0xBA, D|S4K, 4,  64),
	PROPERTIES(0xC0, D|S4K, 4,   8),
	PROPERTIES(0xC0, D|S4M, 4,   8),
	PROPERTIES(0xCA,   S4K|L2, 4, 512),
};
#undef PROPERTIES

static void DecodeDescriptor(u8 descriptor)
{
	// note: we can't use bsearch because propertyTable may contain multiple
	// entries with the same descriptor key.
	for(size_t i = 0; i < ARRAY_SIZE(propertyTable); i++)
	{
		const Properties& properties = propertyTable[i];
		if(properties.descriptor != descriptor)
			continue;

		const size_t flags = properties.flags;
		x86_x64_CacheType type = X86_X64_CACHE_TYPE_UNIFIED;
		if(flags & D)
			type = X86_X64_CACHE_TYPE_DATA;
		else if(flags & I)
			type = X86_X64_CACHE_TYPE_INSTRUCTION;
		const size_t level = (flags & L2)? 2 : 1;
		size_t pageSize = 0;
		if(flags & S4K)
			pageSize = 4*KiB;
		else if(flags & S4M)
			pageSize = 4*MiB;
		else if(flags & S2M)
			pageSize = 2*MiB;
		else
			debug_assert(0);

		x86_x64_TLB tlb;
		tlb.type          = type;
		tlb.level         = level;
		tlb.associativity = properties.associativity;
		tlb.pageSize      = pageSize;
		tlb.entries       = properties.entries;

		if(IsInstruction(type))
		{
			if(itlbs.numLevels < maxTLBLevels)
				itlbs.levels[itlbs.numLevels++] = tlb;
			else
				debug_assert(0);
		}
		if(IsData(type))
		{
			if(dtlbs.numLevels < maxTLBLevels)
				dtlbs.levels[dtlbs.numLevels++] = tlb;
			else
				debug_assert(0);
		}

		return;	// success
	}
}

static void DetectTLB_CPUID2()
{
	// TODO: ensure we are pinned to the same CPU

	// extract descriptors
	x86_x64_CpuidRegs regs = { 0 };
	regs.eax = 2;
	if(!x86_x64_cpuid(&regs))
		return;
	size_t iterations = bits(regs.eax, 0, 7);
	std::vector<u8> descriptors;
	for(;;)
	{
		ExtractDescriptors(bits(regs.eax, 8, 31), descriptors);
		ExtractDescriptors(regs.ebx, descriptors);
		ExtractDescriptors(regs.ecx, descriptors);
		ExtractDescriptors(regs.edx, descriptors);
		if(--iterations == 0)
			break;
		regs.eax = 2;
		const bool ok = x86_x64_cpuid(&regs);
		debug_assert(ok);
	}

	for(std::vector<u8>::const_iterator it = descriptors.begin(); it != descriptors.end(); ++it)
	{
		const u8 descriptor = *it;
		DecodeDescriptor(descriptor);
	}
}

static ModuleInitState cacheInitState;

static LibError DetectCacheAndTLB()
{
	if(x86_x64_Vendor() == X86_X64_VENDOR_AMD)
		AMD::DetectCacheAndTLB();
	else
	{
		DetectCache_CPUID4();
		DetectTLB_CPUID2();
	}

	// sanity check: cache type must match that of the data structure
	for(size_t i = 0; i < dcaches.numLevels; i++)
		debug_assert(dcaches.levels[i].type != X86_X64_CACHE_TYPE_INSTRUCTION);
	for(size_t i = 0; i < icaches.numLevels; i++)
		debug_assert(icaches.levels[i].type != X86_X64_CACHE_TYPE_DATA);
	for(size_t i = 0; i < dtlbs.numLevels; i++)
		debug_assert(dtlbs.levels[i].type != X86_X64_CACHE_TYPE_INSTRUCTION);
	for(size_t i = 0; i < itlbs.numLevels; i++)
		debug_assert(itlbs.levels[i].type != X86_X64_CACHE_TYPE_DATA);

	// ensure x86_x64_L1CacheLineSize and x86_x64_L2CacheLineSize will work
	debug_assert(dcaches.numLevels >= 2);
	debug_assert(dcaches.levels[0].lineSize != 0);
	debug_assert(dcaches.levels[1].lineSize != 0);

	return INFO::OK;
}

const x86_x64_Caches* x86_x64_ICaches()
{
	ModuleInit(&cacheInitState, DetectCacheAndTLB);
	return &icaches;
}

const x86_x64_Caches* x86_x64_DCaches()
{
	ModuleInit(&cacheInitState, DetectCacheAndTLB);
	return &dcaches;
}

size_t x86_x64_L1CacheLineSize()
{
	return x86_x64_DCaches()->levels[0].lineSize;
}

size_t x86_x64_L2CacheLineSize()
{
	return x86_x64_DCaches()->levels[1].lineSize;
}

const x86_x64_TLBs* x86_x64_ITLBs()
{
	ModuleInit(&cacheInitState, DetectCacheAndTLB);
	return &itlbs;
}

const x86_x64_TLBs* x86_x64_DTLBs()
{
	ModuleInit(&cacheInitState, DetectCacheAndTLB);
	return &dtlbs;
}

size_t x86_x64_TLBCoverage(const x86_x64_TLBs* tlbs)
{
	// note: receiving a TLB pointer means DetectCacheAndTLB was called.

	const u64 pageSize = 4*KiB;
	const u64 largePageSize = os_cpu_LargePageSize();
	u64 totalSize = 0;	// [bytes]
	for(size_t i = 0; i < tlbs->numLevels; i++)
	{
		const x86_x64_TLB& tlb = tlbs->levels[i];
		if(tlb.pageSize == pageSize)
			totalSize += pageSize * tlb.entries;
		if(tlb.pageSize == largePageSize)
			totalSize += largePageSize * tlb.entries;
	}

	return size_t(totalSize / MiB);
}


//-----------------------------------------------------------------------------
// identifier string

/// functor to remove substrings from the CPU identifier string
class StringStripper
{
public:
	StringStripper(char* string, size_t max_chars)
		: m_string(string), m_max_chars(max_chars)
	{
	}

	// remove all instances of substring from m_string
	void operator()(const char* substring)
	{
		const size_t substring_length = strlen(substring);
		for(;;)
		{
			char* substring_pos = strstr(m_string, substring);
			if(!substring_pos)
				break;
			const size_t substring_ofs = substring_pos - m_string;
			const size_t num_chars = m_max_chars - substring_ofs - substring_length;
			memmove(substring_pos, substring_pos+substring_length, num_chars);
		}
	}

private:
	char* m_string;
	size_t m_max_chars;
};

// 3 calls x 4 registers x 4 bytes = 48 + 0-terminator
static char identifierString[48+1];

static LibError InitIdentifierString()
{
	// get brand string (if available)
	char* pos = identifierString;
	bool gotBrandString = true;
	for(u32 function = 0x80000002; function <= 0x80000004; function++)
	{
		x86_x64_CpuidRegs regs = { 0 };
		regs.eax = function;
		gotBrandString &= x86_x64_cpuid(&regs);
		memcpy(pos, &regs, 16);
		pos += 16;
	}

	// fall back to manual detect of CPU type because either:
	// - CPU doesn't support brand string (we use a flag to indicate this
	//   rather than comparing against a default value because it is safer);
	// - the brand string is useless, e.g. "Unknown". this happens on
	//   some older boards whose BIOS reprograms the string for CPUs it
	//   doesn't recognize.
	if(!gotBrandString || strncmp(identifierString, "Unknow", 6) == 0)
	{
		const size_t family = x86_x64_Family();
		const size_t model = x86_x64_Model();
		switch(x86_x64_Vendor())
		{
		case X86_X64_VENDOR_AMD:
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 3 || model == 7)
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "AMD Duron");
				else if(model <= 5)
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "AMD Athlon");
				else
				{
					if(x86_x64_cap(X86_X64_CAP_AMD_MP))
						strcpy_s(identifierString, ARRAY_SIZE(identifierString), "AMD Athlon MP");
					else
						strcpy_s(identifierString, ARRAY_SIZE(identifierString), "AMD Athlon XP");
				}
			}
			break;

		case X86_X64_VENDOR_INTEL:
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 1)
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "Intel Pentium Pro");
				else if(model == 3 || model == 5)
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "Intel Pentium II");
				else if(model == 6)
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "Intel Celeron");	
				else
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "Intel Pentium III");
			}
			break;

		default:
			strcpy_s(identifierString, ARRAY_SIZE(identifierString), "Unknown, non-Intel/AMD");
			break;
		}
	}
	// identifierString already holds a valid brand string; pretty it up.
	else
	{
		const char* const undesiredStrings[] = { "(tm)", "(TM)", "(R)", "CPU ", "          " };
		std::for_each(undesiredStrings, undesiredStrings+ARRAY_SIZE(undesiredStrings),
			StringStripper(identifierString, strlen(identifierString)+1));

		// note: Intel brand strings include a frequency, but we can't rely
		// on it because the CPU may be overclocked. we'll leave it in the
		// string to show measurement accuracy and if SpeedStep is active.
	}

	return INFO::OK;
}

const char* cpu_IdentifierString()
{
	static ModuleInitState initState;
	ModuleInit(&initState, InitIdentifierString);
	return identifierString;
}


//-----------------------------------------------------------------------------
// miscellaneous stateless functions

// these routines do not call ModuleInit (because some of them are
// time-critical, e.g. cpu_Serialize) and should also avoid the
// other x86_x64* functions and their global state.
// in particular, use cpuid instead of x86_x64_cpuid.

u8 x86_x64_ApicId()
{
	x86_x64_CpuidRegs regs = { 0 };
	regs.eax = 1;
	// note: CPUID function 1 is always supported, but only processors with
	// an xAPIC (e.g. P4/Athlon XP) will return a nonzero ID.
	cpuid(&regs);
	const u8 apicId = (u8)bits(regs.ebx, 24, 31);
	return apicId;
}


#if !MSC_VERSION	// replaced by macro
u64 x86_x64_rdtsc()
{
#if GCC_VERSION
	// GCC supports "portable" assembly for both x86 and x64
	volatile u32 lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return u64_from_u32(hi, lo);
#endif
}
#endif


void x86_x64_DebugBreak()
{
#if MSC_VERSION
	__debugbreak();
#elif GCC_VERSION
	// note: this probably isn't necessary, since unix_debug_break
	// (SIGTRAP) is most probably available if GCC_VERSION.
	// we include it for completeness, though.
	__asm__ __volatile__ ("int $3");
#endif
}


void cpu_Serialize()
{
	x86_x64_CpuidRegs regs = { 0 };
	regs.eax = 1;
	cpuid(&regs);	// CPUID serializes execution.
}


//-----------------------------------------------------------------------------
// CPU frequency

// set scheduling priority and restore when going out of scope.
class ScopedSetPriority
{
public:
	ScopedSetPriority(int newPriority)
	{
		// get current scheduling policy and priority
		pthread_getschedparam(pthread_self(), &m_oldPolicy, &m_oldParam);

		// set new priority
		sched_param newParam = {0};
		newParam.sched_priority = newPriority;
		pthread_setschedparam(pthread_self(), SCHED_FIFO, &newParam);
	}

	~ScopedSetPriority()
	{
		// restore previous policy and priority.
		pthread_setschedparam(pthread_self(), m_oldPolicy, &m_oldParam);
	}

private:
	int m_oldPolicy;
	sched_param m_oldParam;
};

// note: this function uses timer.cpp!timer_Time, which is implemented via
// whrt.cpp on Windows.
double x86_x64_ClockFrequency()
{
	// if the TSC isn't available, there's really no good way to count the
	// actual CPU clocks per known time interval, so bail.
	// note: loop iterations ("bogomips") are not a reliable measure due
	// to differing IPC and compiler optimizations.
	if(!x86_x64_cap(X86_X64_CAP_TSC))
		return -1.0;	// impossible value

	// increase priority to reduce interference while measuring.
	const int priority = sched_get_priority_max(SCHED_FIFO)-1;
	ScopedSetPriority ssp(priority);

	// note: no need to "warm up" cpuid - it will already have been
	// called several times by the time this code is reached.
	// (background: it's used in x86_x64_rdtsc() to serialize instruction flow;
	// the first call is documented to be slower on Intel CPUs)

	size_t numSamples = 16;
	// if clock is low-res, do less samples so it doesn't take too long.
	// balance measuring time (~ 10 ms) and accuracy (< 1 0/00 error -
	// ok for using the TSC as a time reference)
	if(timer_Resolution() >= 1e-3)
		numSamples = 8;
	std::vector<double> samples(numSamples);

	for(size_t i = 0; i < numSamples; i++)
	{
		double dt;
		i64 dc;	// (i64 instead of u64 for faster conversion to double)

		// count # of clocks in max{1 tick, 1 ms}:
		// .. wait for start of tick.
		const double t0 = timer_Time();
		u64 c1; double t1;
		do
		{
			// note: timer_Time effectively has a long delay (up to 5 us)
			// before returning the time. we call it before x86_x64_rdtsc to
			// minimize the delay between actually sampling time / TSC,
			// thus decreasing the chance for interference.
			// (if unavoidable background activity, e.g. interrupts,
			// delays the second reading, inaccuracy is introduced).
			t1 = timer_Time();
			c1 = x86_x64_rdtsc();
		}
		while(t1 == t0);
		// .. wait until start of next tick and at least 1 ms elapsed.
		do
		{
			const double t2 = timer_Time();
			const u64 c2 = x86_x64_rdtsc();
			dc = (i64)(c2 - c1);
			dt = t2 - t1;
		}
		while(dt < 1e-3);

		// .. freq = (delta_clocks) / (delta_seconds);
		//    x86_x64_rdtsc/timer overhead is negligible.
		const double freq = dc / dt;
		samples[i] = freq;
	}

	std::sort(samples.begin(), samples.end());

	// median filter (remove upper and lower 25% and average the rest).
	// note: don't just take the lowest value! it could conceivably be
	// too low, if background processing delays reading c1 (see above).
	double sum = 0.0;
	const size_t lo = numSamples/4, hi = 3*numSamples/4;
	for(size_t i = lo; i < hi; i++)
		sum += samples[i];

	const double clockFrequency = sum / (hi-lo);
	return clockFrequency;
}
