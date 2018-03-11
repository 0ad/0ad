/* Copyright (C) 2018 Wildfire Games.
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

#include "precompiled.h"
#include "lib/sysdep/arch/x86_x64/cache.h"

#include "lib/bits.h"
#include "lib/alignment.h"
#include "lib/module_init.h"
#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/arch/x86_x64/x86_x64.h"

namespace x86_x64 {

static const size_t maxTLBs = 2*2*4;	// (level0, level1) x (D,I) x (4K, 2M, 4M, 1G)
static size_t numTLBs = 0;

static const size_t numCaches = x86_x64::Cache::maxLevels * 2 + maxTLBs;
static Cache caches[numCaches];


static void AddCache(const x86_x64::Cache& cache)
{
	ENSURE(cache.Validate());

	if(cache.m_Type == x86_x64::Cache::kData || cache.m_Type == x86_x64::Cache::kUnified)
		caches[L1D + cache.m_Level-1] = cache;
	if(cache.m_Type == x86_x64::Cache::kInstruction || cache.m_Type == x86_x64::Cache::kUnified)
		caches[L1I + cache.m_Level-1] = cache;
}


static void AddTLB(const x86_x64::Cache& tlb)
{
	ENSURE(tlb.Validate());
	ENSURE(tlb.m_Level == 1 || tlb.m_Level == 2);	// see maxTLBs

	ENSURE(numTLBs < maxTLBs);
	caches[TLB+numTLBs++] = tlb;
}


//-----------------------------------------------------------------------------
// AMD

// (Intel has subsequently added support for function 0x80000006, but
// only returns ECX, i.e. L2 information.)
namespace AMD
{

static x86_x64::Cache L1Cache(u32 reg, x86_x64::Cache::Type type)
{
	x86_x64::Cache cache;
	cache.Initialize(1, type);

	const size_t lineSize      = bits(reg,  0,  7);
	const size_t associativity = bits(reg, 16, 23);	// 0 = reserved
	const size_t totalSize     = bits(reg, 24, 31)*KiB;
	if(lineSize != 0 && associativity != 0 && totalSize != 0)
	{
		cache.m_NumEntries = totalSize / lineSize;
		cache.m_EntrySize = lineSize;
		cache.m_Associativity = associativity;
		cache.m_SharedBy = 1;
	}
	return cache;
}

// applies to L2, L3 and TLB2
static const size_t associativityTable[16] =
{
	0, 1, 2, 0, 4, 0, 8, 0,
	16, 0, 32, 48, 64, 96, 128, x86_x64::Cache::fullyAssociative
};

static x86_x64::Cache L2Cache(u32 reg, x86_x64::Cache::Type type)
{
	x86_x64::Cache cache;
	cache.Initialize(2, type);

	const size_t lineSize         = bits(reg,  0,  7);
	const size_t idxAssociativity = bits(reg, 12, 15);	// 0 = disabled
	const size_t totalSize        = bits(reg, 16, 31)*KiB;
	if(lineSize != 0 && idxAssociativity != 0 && totalSize != 0)
	{
		cache.m_NumEntries = totalSize / lineSize;
		cache.m_EntrySize = lineSize;
		cache.m_Associativity = associativityTable[idxAssociativity];
		cache.m_SharedBy = 1;
	}
	return cache;
}

// (same as L2 except for the size)
static x86_x64::Cache L3Cache(u32 reg, x86_x64::Cache::Type type)
{
	x86_x64::Cache cache;
	cache.Initialize(3, type);

	const size_t lineSize         = bits(reg,  0,  7);
	const size_t idxAssociativity = bits(reg, 12, 15);	// 0 = disabled
	const size_t totalSize        = bits(reg, 18, 31)*512*KiB;	// (rounded down)
	// NB: some Athlon 64 X2 models have no L3 cache
	if(lineSize != 0 && idxAssociativity != 0 && totalSize != 0)
	{
		cache.m_NumEntries = totalSize / lineSize;
		cache.m_EntrySize = lineSize;
		cache.m_Associativity = associativityTable[idxAssociativity];
		cache.m_SharedBy = 1;
	}
	return cache;
}

static x86_x64::Cache TLB1(u32 reg, size_t bitOffset, size_t pageSize, x86_x64::Cache::Type type)
{
	x86_x64::Cache cache;
	cache.Initialize(1, type);

	const size_t numEntries    = bits(reg, bitOffset+0, bitOffset+ 7);
	const size_t associativity = bits(reg, bitOffset+8, bitOffset+15);	// 0 = reserved
	if(numEntries != 0 && associativity != 0)
	{
		cache.m_NumEntries = numEntries;
		cache.m_EntrySize = pageSize;
		cache.m_Associativity = associativity;
		cache.m_SharedBy = 1;
	}
	return cache;
}

static x86_x64::Cache TLB2(u32 reg, size_t bitOffset, size_t pageSize, x86_x64::Cache::Type type)
{
	x86_x64::Cache cache;
	cache.Initialize(2, type);

	const size_t numEntries       = bits(reg, bitOffset+ 0, bitOffset+11);
	const size_t idxAssociativity = bits(reg, bitOffset+12, bitOffset+15);	// 0 = disabled
	if(numEntries != 0 && idxAssociativity != 0)
	{
		cache.m_NumEntries = numEntries;
		cache.m_EntrySize = pageSize;
		cache.m_Associativity = associativityTable[idxAssociativity];
		cache.m_SharedBy = 1;
	}
	return cache;
}

static void AddTLB2Pair(u32 reg, size_t pageSize)
{
	x86_x64::Cache::Type type = x86_x64::Cache::kUnified;
	if(bits(reg, 16, 31) != 0)	// not unified
	{
		AddTLB(TLB2(reg, 16, pageSize, x86_x64::Cache::kData));
		type = x86_x64::Cache::kInstruction;
	}
	AddTLB(TLB2(reg, 0, pageSize, type));
}

// AMD reports maxCpuidIdFunction > 4 but consider functions 2..4 to be
// "reserved". cache characteristics are returned via ext. functions.
static void DetectCacheAndTLB()
{
	x86_x64::CpuidRegs regs = { 0 };

	regs.eax = 0x80000005;
	if(x86_x64::cpuid(&regs))
	{
		AddCache(L1Cache(regs.ecx, x86_x64::Cache::kData));
		AddCache(L1Cache(regs.edx, x86_x64::Cache::kInstruction));

		AddTLB(TLB1(regs.eax,  0, 2*MiB, x86_x64::Cache::kInstruction));
		AddTLB(TLB1(regs.eax, 16, 2*MiB, x86_x64::Cache::kData));
		AddTLB(TLB1(regs.ebx,  0, 4*KiB, x86_x64::Cache::kInstruction));
		AddTLB(TLB1(regs.ebx, 16, 4*KiB, x86_x64::Cache::kData));
	}

	regs.eax = 0x80000006;
	if(x86_x64::cpuid(&regs))
	{
		AddCache(L2Cache(regs.ecx, x86_x64::Cache::kUnified));
		AddCache(L3Cache(regs.edx, x86_x64::Cache::kUnified));

		AddTLB2Pair(regs.eax, 2*MiB);
		AddTLB2Pair(regs.ebx, 4*KiB);
	}
}

}	// namespace AMD


//-----------------------------------------------------------------------------
// CPUID.4

namespace CPUID4 {

static bool DetectCache()
{
	// note: level order is unspecified (see Intel AP-485)
	for(u32 count = 0; ; count++)
	{
		x86_x64::CpuidRegs regs = { 0 };
		regs.eax = 4;
		regs.ecx = count;
		if(!x86_x64::cpuid(&regs))
			return false;

		const x86_x64::Cache::Type type = (x86_x64::Cache::Type)bits(regs.eax, 0, 4);
		if(type == x86_x64::Cache::kNull)	// no more remaining
			break;

		const size_t level      = (size_t)bits(regs.eax, 5, 7);
		const size_t partitions = (size_t)bits(regs.ebx, 12, 21)+1;
		const size_t sets       = (size_t)bits(regs.ecx, 0, 31)+1;

		x86_x64::Cache cache;
		cache.Initialize(level, type);
		cache.m_EntrySize = static_cast<size_t>(bits(regs.ebx, 0, 11) + 1); // (yes, this also uses +1 encoding)
		cache.m_Associativity = static_cast<size_t>(bits(regs.ebx, 22, 31) + 1);
		cache.m_SharedBy = static_cast<size_t>(bits(regs.eax, 14, 25) + 1);
		cache.m_NumEntries = cache.m_Associativity * partitions * sets;

		AddCache(cache);
	}

	return true;
}

}	// namespace CPUID4


//-----------------------------------------------------------------------------
// CPUID.2 (descriptors)

namespace CPUID2 {

typedef u8 Descriptor;
typedef std::vector<Descriptor> Descriptors;

static void AppendDescriptors(u32 reg, Descriptors& descriptors)
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


static Descriptors GetDescriptors()
{
	// ensure consistency by pinning to a CPU.
	// (don't use a hard-coded mask because process affinity may be restricted)
	const uintptr_t allProcessors = os_cpu_ProcessorMask();
	const uintptr_t firstProcessor = allProcessors & -intptr_t(allProcessors);
	const uintptr_t prevAffinityMask = os_cpu_SetThreadAffinityMask(firstProcessor);

	x86_x64::CpuidRegs regs = { 0 };
	regs.eax = 2;
	if(!x86_x64::cpuid(&regs))
		return Descriptors();

	Descriptors descriptors;
	size_t iterations = bits(regs.eax, 0, 7);
	for(;;)	// abort mid-loop (invoke CPUID exactly <iterations> times)
	{
		AppendDescriptors(bits(regs.eax, 8, 31), descriptors);
		AppendDescriptors(regs.ebx, descriptors);
		AppendDescriptors(regs.ecx, descriptors);
		AppendDescriptors(regs.edx, descriptors);
		if(--iterations == 0)
			break;
		regs.eax = 2;
		const bool ok = x86_x64::cpuid(&regs);
		ENSURE(ok);
	}

	os_cpu_SetThreadAffinityMask(prevAffinityMask);

	return descriptors;
}


// note: the following cannot be moved into a function because
// ARRAY_SIZE's template argument must not reference a local type.

enum Flags
{
	// level (bits 0..1)
	L1 = 1,
	L2,
	L3,

	// type (bits 2..3)
	I   = 0x04,	// instruction
	D   = 0x08,	// data
	U   = I|D	// unified

	// largeSize (bits 4..31 with bits 0..3 zeroed): TLB entrySize or cache numEntries
};

// (there are > 100 descriptors, so we squeeze all fields into 8 bytes.)
struct Characteristics	// POD
{
	x86_x64::Cache::Type Type() const
	{
		switch(flags & U)
		{
		case D:
			return x86_x64::Cache::kData;
		case I:
			return x86_x64::Cache::kInstruction;
		case U:
			return x86_x64::Cache::kUnified;
		default:
			DEBUG_WARN_ERR(ERR::LOGIC);
			return x86_x64::Cache::kNull;
		}
	}

	size_t Level() const
	{
		const size_t level = flags & 3;
		ENSURE(level != 0);
		return level;
	}

	bool IsTLB() const
	{
		return smallSize >= 0;
	}

	size_t NumEntries() const
	{
		return IsTLB()? (size_t)smallSize : (flags & ~0xF);
	}

	size_t EntrySize() const
	{
		return IsTLB()? (flags & ~0xF) : (size_t)(-smallSize);
	}

	u8 descriptor;
	u8 associativity;
	i16 smallSize;	// negative cache entrySize or TLB numEntries
	u32 flags;	// level, type, largeSize
};

static const u8 F = x86_x64::Cache::fullyAssociative;

#define CACHE(descriptor, flags, totalSize, assoc, entrySize)  { descriptor, assoc, -entrySize, flags | ((totalSize)/(entrySize)) }
#define TLB(descriptor, flags, entrySize, assoc, numEntries) { descriptor, assoc, numEntries, flags | (entrySize) }

// (we need to include cache descriptors because early Pentium4 don't implement CPUID.4)
// references: [accessed 2011-02-26]
// AP485 http://www.intel.com/Assets/PDF/appnote/241618.pdf
// sdman http://www.intel.com/Assets/PDF/manual/253666.pdf
// sandp http://www.sandpile.org/ia32/cpuid.htm
// opsol http://src.opensolaris.org/source/xref/onnv/onnv-gate/usr/src/uts/i86pc/os/cpuid.c
static const Characteristics characteristicsTable[] =
{
	TLB  (0x01, L1|I,   4*KiB,  4,  32),
	TLB  (0x02, L1|I,   4*MiB,  F,   2),
	TLB  (0x03, L1|D,   4*KiB,  4,  64),
	TLB  (0x04, L1|D,   4*MiB,  4,   8),
	TLB  (0x05, L1|D,   4*MiB,  4,  32),

	CACHE(0x06, L1|I,   8*KiB,  4,  32),
	CACHE(0x08, L1|I,  16*KiB,  4,  32),
	CACHE(0x09, L1|I,  32*KiB,  4,  64),
	CACHE(0x0A, L1|I,   8*KiB,  2,  32),

	TLB  (0x0B, L1|I,   4*MiB,  4,   4),

	CACHE(0x0C, L1|D,  16*KiB,  4,  32),
	CACHE(0x0D, L1|D,  16*KiB,  4,  64),	// opsol: 32B (would be redundant with 0x0C), AP485: 64B, sdman: 64B
	CACHE(0x0E, L1|D,  24*KiB,  6,  64),

	CACHE(0x21, L2|U, 256*KiB,  8,  64),

	CACHE(0x22, L3|U, 512*KiB,  4,  64),
	CACHE(0x23, L3|U,   1*MiB,  8,  64),
	CACHE(0x25, L3|U,   2*MiB,  8,  64),
	CACHE(0x29, L3|U,   4*MiB,  8,  64),

	CACHE(0x2c, L1|D,  32*KiB,  8,  64),

	CACHE(0x30, L1|I,  32*KiB,  8,  64),

	CACHE(0x39, L2|U, 128*KiB,  4,  64),
	CACHE(0x3A, L2|U, 192*KiB,  6,  64),
	CACHE(0x3B, L2|U, 128*KiB,  2,  64),
	CACHE(0x3C, L2|U, 256*KiB,  4,  64),
	CACHE(0x3D, L2|U, 384*KiB,  6,  64),
	CACHE(0x3E, L2|U, 512*KiB,  4,  64),
	CACHE(0x41, L2|U, 128*KiB,  4,  32),
	CACHE(0x42, L2|U, 256*KiB,  4,  32),
	CACHE(0x43, L2|U, 512*KiB,  4,  32),
	CACHE(0x44, L2|U,   1*MiB,  4,  32),
	CACHE(0x45, L2|U,   2*MiB,  4,  32),

	CACHE(0x46, L3|U,   4*MiB,  4,  64),
	CACHE(0x47, L3|U,   8*MiB,  8,  64),
	CACHE(0x48, L2|U,   3*MiB, 12,  64),
	CACHE(0x49, L2|U,   4*MiB, 16,  64),
	CACHE(0x49, L3|U,   4*MiB, 16,  64),
	CACHE(0x4A, L3|U,   6*MiB, 12,  64),
	CACHE(0x4B, L3|U,   8*MiB, 16,  64),
	CACHE(0x4C, L3|U,  12*MiB, 12,  64),
	CACHE(0x4D, L3|U,  16*MiB, 16,  64),
	CACHE(0x4E, L2|U,   6*MiB, 24,  64),

	TLB  (0x4F, L1|I,   4*KiB,  F,  32),	// sandp: unknown assoc, opsol: full, AP485: unspecified
	TLB  (0x50, L1|I,   4*KiB,  F,  64),
	TLB  (0x50, L1|I,   4*MiB,  F,  64),
	TLB  (0x50, L1|I,   2*MiB,  F,  64),
	TLB  (0x51, L1|I,   4*KiB,  F, 128),
	TLB  (0x51, L1|I,   4*MiB,  F, 128),
	TLB  (0x51, L1|I,   2*MiB,  F, 128),
	TLB  (0x52, L1|I,   4*KiB,  F, 256),
	TLB  (0x52, L1|I,   4*MiB,  F, 256),
	TLB  (0x52, L1|I,   2*MiB,  F, 256),
	TLB  (0x55, L1|I,   4*MiB,  F,   7),
	TLB  (0x55, L1|I,   2*MiB,  F,   7),

	TLB  (0x56, L1|D,   4*MiB,  4,  16),
	TLB  (0x57, L1|D,   4*KiB,  4,  16),
	TLB  (0x59, L1|D,   4*KiB,  F,  16),
	TLB  (0x5A, L1|D,   4*MiB,  4,  32),
	TLB  (0x5A, L1|D,   2*MiB,  4,  32),
	TLB  (0x5B, L1|D,   4*KiB,  F,  64),
	TLB  (0x5B, L1|D,   4*MiB,  F,  64),
	TLB  (0x5C, L1|D,   4*KiB,  F, 128),
	TLB  (0x5C, L1|D,   4*MiB,  F, 128),
	TLB  (0x5D, L1|D,   4*KiB,  F, 256),
	TLB  (0x5D, L1|D,   4*MiB,  F, 256),

	CACHE(0x60, L1|D,  16*KiB,  8,  64),
	TLB  (0x63, L1|D,   1*GiB,  4,   4),    // speculation
	CACHE(0x66, L1|D,   8*KiB,  4,  64),
	CACHE(0x67, L1|D,  16*KiB,  4,  64),
	CACHE(0x68, L1|D,  32*KiB,  4,  64),

	CACHE(0x70, L1|I,  12*KiB,  8,   1),
	CACHE(0x71, L1|I,  16*KiB,  8,   1),
	CACHE(0x72, L1|I,  32*KiB,  8,   1),
	CACHE(0x73, L1|I,  64*KiB,  8,   1),

	TLB  (0x76, L1|I,   4*MiB,  F,   8),	// AP485: internally inconsistent, sdman: TLB
	TLB  (0x76, L1|I,   2*MiB,  F,   8),

	CACHE(0x78, L2|U,   1*MiB,  4,  64),
	CACHE(0x79, L2|U, 128*KiB,  8,  64),
	CACHE(0x7A, L2|U, 256*KiB,  8,  64),
	CACHE(0x7B, L2|U, 512*KiB,  8,  64),
	CACHE(0x7C, L2|U,   1*MiB,  8,  64),
	CACHE(0x7D, L2|U,   2*MiB,  8,  64),
	CACHE(0x7F, L2|U, 512*KiB,  2,  64),

	CACHE(0x80, L2|U, 512*KiB,  8,  64),
	CACHE(0x82, L2|U, 256*KiB,  8,  32),
	CACHE(0x83, L2|U, 512*KiB,  8,  32),
	CACHE(0x84, L2|U,   1*MiB,  8,  32),
	CACHE(0x85, L2|U,   2*MiB,  8,  32),
	CACHE(0x86, L2|U, 512*KiB,  4,  64),
	CACHE(0x87, L2|U,   1*MiB,  8,  64),

	TLB  (0xB0, L1|I,   4*KiB,  4, 128),
	TLB  (0xB1, L1|I,   2*MiB,  4,   8),
	TLB  (0xB1, L1|I,   4*MiB,  4,   4),
	TLB  (0xB2, L1|I,   4*KiB,  4,  64),

	TLB  (0xB3, L1|D,   4*KiB,  4, 128),
	TLB  (0xB3, L1|D,   4*MiB,  4, 128),
	TLB  (0xB4, L1|D,   4*KiB,  4, 256),
	TLB  (0xB4, L1|D,   4*MiB,  4, 256),
	TLB  (0xB5, L1|I,   4*KiB,  4, 128),    // speculation
	TLB  (0xB6, L1|I,   4*KiB,  8, 128),    // http://software.intel.com/en-us/forums/topic/401012

	TLB  (0xBA, L1|D,   4*KiB,  4,  64),
	TLB  (0xC0, L1|D,   4*KiB,  4,   8),
	TLB  (0xC0, L1|D,   4*MiB,  4,   8),
	TLB  (0xC1, L2|U,   4*KiB,  8, 1024),   // http://software.intel.com/en-us/forums/topic/401012
	TLB  (0xC1, L2|U,   4*MiB,  8, 1024),
	TLB  (0xC1, L2|U,   2*MiB,  8, 1024),
	TLB  (0xCA, L2|U,   4*KiB,  4, 512),

	CACHE(0xD0, L3|U, 512*KiB,  4,  64),
	CACHE(0xD1, L3|U,   1*MiB,  4,  64),
	CACHE(0xD2, L3|U,   2*MiB,  4,  64),
	CACHE(0xD6, L3|U,   1*MiB,  8,  64),
	CACHE(0xD7, L3|U,   2*MiB,  8,  64),
	CACHE(0xD8, L3|U,   4*MiB,  8,  64),
	CACHE(0xDC, L3|U, 3*MiB/2, 12,  64),
	CACHE(0xDD, L3|U,   3*MiB, 12,  64),
	CACHE(0xDE, L3|U,   6*MiB, 12,  64),
	CACHE(0xE2, L3|U,   2*MiB, 16,  64),
	CACHE(0xE3, L3|U,   4*MiB, 16,  64),
	CACHE(0xE4, L3|U,   8*MiB, 16,  64),
	CACHE(0xEA, L3|U,  12*MiB, 24,  64),
	CACHE(0xEB, L3|U,  18*MiB, 24,  64),
	CACHE(0xEC, L3|U,  24*MiB, 24,  64),
};
#undef CACHE
#undef TLB

static const Characteristics* CharacteristicsFromDescriptor(Descriptor descriptor)
{
	// note: we can't use bsearch because characteristicsTable contains multiple
	// entries with the same descriptor.
	for(size_t i = 0; i < ARRAY_SIZE(characteristicsTable); i++)
	{
		const Characteristics& characteristics = characteristicsTable[i];
		if(characteristics.descriptor == descriptor)
			return &characteristics;
	}

	debug_printf("Unknown cache/TLB descriptor 0x%x\n", (unsigned int)descriptor);
	return 0;
}


enum DescriptorFlags
{
	SKIP_CACHE_DESCRIPTORS = 1,
	NO_LAST_LEVEL_CACHE    = 2,
	PREFETCH64             = 64,
	PREFETCH128            = 128
};

static bool HandleSpecialDescriptor(Descriptor descriptor, size_t& descriptorFlags)
{
	switch(descriptor)
	{
	case 0:	// carries no information
		return true;

	case 0x40:
		descriptorFlags |= NO_LAST_LEVEL_CACHE;
		return true;

	case 0xF0:
		descriptorFlags |= PREFETCH64;
		return true;

	case 0xF1:
		descriptorFlags |= PREFETCH128;
		return true;

	case 0xFF:	// descriptors don't include caches (use CPUID.4 instead)
		descriptorFlags |= SKIP_CACHE_DESCRIPTORS;
		return true;

	default:
		return false;
	}
}


static void DetectCacheAndTLB(size_t& descriptorFlags)
{
	const Descriptors descriptors = GetDescriptors();
	for(Descriptors::const_iterator it = descriptors.begin(); it != descriptors.end(); ++it)
	{
		const Descriptor descriptor = *it;
		if(HandleSpecialDescriptor(descriptor, descriptorFlags))
			continue;

		const Characteristics* characteristics = CharacteristicsFromDescriptor(*it);
		if(!characteristics)
			continue;

		if((descriptorFlags & SKIP_CACHE_DESCRIPTORS) && !characteristics->IsTLB())
			continue;

		x86_x64::Cache cache;
		cache.Initialize(characteristics->Level(), characteristics->Type());
		cache.m_NumEntries = characteristics->NumEntries();
		cache.m_EntrySize = characteristics->EntrySize();
		cache.m_Associativity = characteristics->associativity;
		cache.m_SharedBy = 1;	// (safe default)
		if(characteristics->IsTLB())
			AddTLB(cache);
		else
			AddCache(cache);
	}
}

}	// namespace CPUID2


static Status DetectCacheAndTLB()
{
	// ensure all cache entries are initialized (DetectCache* might not set them all)
	for(size_t idxLevel = 0; idxLevel < x86_x64::Cache::maxLevels; idxLevel++)
	{
		caches[L1D+idxLevel].Initialize(idxLevel+1, x86_x64::Cache::kData);
		caches[L1I+idxLevel].Initialize(idxLevel+1, x86_x64::Cache::kInstruction);
	}

	if(x86_x64::Vendor() == x86_x64::VENDOR_AMD)
		AMD::DetectCacheAndTLB();
	else
	{
		size_t descriptorFlags = 0;
		if(CPUID4::DetectCache())	// success, ignore less reliable CPUID.2 cache information
			descriptorFlags |= CPUID2::SKIP_CACHE_DESCRIPTORS;
		CPUID2::DetectCacheAndTLB(descriptorFlags);
	}

	// sanity checks
	for(size_t idxLevel = 0; idxLevel < x86_x64::Cache::maxLevels; idxLevel++)
	{
		ENSURE(caches[L1D+idxLevel].m_Type == x86_x64::Cache::kData || caches[L1D+idxLevel].m_Type == x86_x64::Cache::kUnified);
		ENSURE(caches[L1D+idxLevel].m_Level == idxLevel+1);
		ENSURE(caches[L1D+idxLevel].Validate() == true);

		ENSURE(caches[L1I+idxLevel].m_Type == x86_x64::Cache::kInstruction || caches[L1I+idxLevel].m_Type == x86_x64::Cache::kUnified);
		ENSURE(caches[L1I+idxLevel].m_Level == idxLevel+1);
		ENSURE(caches[L1I+idxLevel].Validate() == true);
	}
	for(size_t i = 0; i < numTLBs; i++)
		ENSURE(caches[TLB+i].Validate() == true);

	return INFO::OK;
}

const x86_x64::Cache* Caches(size_t idxCache)
{
	static ModuleInitState initState;
	ModuleInit(&initState, DetectCacheAndTLB);

	if(idxCache >= TLB+numTLBs)
		return 0;

	return &caches[idxCache];
}

}	// namespace x86_x64
