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
 * detection of CPU and cache topology
 */

#include "precompiled.h"
#include "lib/sysdep/arch/x86_x64/topology.h"

#include <set>

#include "lib/bits.h"
#include "lib/module_init.h"
#include "lib/sysdep/cpu.h"	// ERR::CPU_FEATURE_MISSING
#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/numa.h"
#include "lib/sysdep/arch/x86_x64/x86_x64.h"
#include "lib/sysdep/arch/x86_x64/cache.h"


//-----------------------------------------------------------------------------
// detect *maximum* number of cores/packages/caches.
// note: some of them may be disabled by the OS or BIOS.
// note: Intel Appnote 485 assures us that they are uniform across packages.

static size_t MaxCoresPerPackage()
{
	// assume single-core unless one of the following applies:
	size_t maxCoresPerPackage = 1;

	x86_x64_CpuidRegs regs = { 0 };
	switch(x86_x64_Vendor())
	{
	case X86_X64_VENDOR_INTEL:
		regs.eax = 4;
		regs.ecx = 0;
		if(x86_x64_cpuid(&regs))
			maxCoresPerPackage = bits(regs.eax, 26, 31)+1;
		break;

	case X86_X64_VENDOR_AMD:
		regs.eax = 0x80000008;
		if(x86_x64_cpuid(&regs))
			maxCoresPerPackage = bits(regs.ecx, 0, 7)+1;
		break;

	default:
		break;
	}

	return maxCoresPerPackage;
}


static size_t MaxLogicalPerCore()
{
	struct IsHyperthreadingCapable
	{
		bool operator()() const
		{
			// definitely not
			if(!x86_x64_cap(X86_X64_CAP_HT))
				return false;

			// multi-core AMD systems falsely set the HT bit for reasons of
			// compatibility. we'll just ignore it, because clearing it might
			// confuse other callers.
			if(x86_x64_Vendor() == X86_X64_VENDOR_AMD && x86_x64_cap(X86_X64_CAP_AMD_CMP_LEGACY))
				return false;

			return true;
		}
	};
	if(IsHyperthreadingCapable()())
	{
		x86_x64_CpuidRegs regs = { 0 };
		regs.eax = 1;
		if(!x86_x64_cpuid(&regs))
			DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
		const size_t logicalPerPackage = bits(regs.ebx, 16, 23);
		const size_t maxCoresPerPackage = MaxCoresPerPackage();
		// cores ought to be uniform WRT # logical processors
		ENSURE(logicalPerPackage % maxCoresPerPackage == 0);
		const size_t maxLogicalPerCore = logicalPerPackage / maxCoresPerPackage;
		return maxLogicalPerCore;
	}
	else
		return 1;
}


static size_t MaxLogicalPerCache()
{
	return x86_x64_Caches(L2D)->sharedBy;
}


//-----------------------------------------------------------------------------
// APIC IDs

// APIC IDs consist of variable-length fields identifying the logical unit,
// core, package and shared cache. if they are available, we can determine
// the exact topology; otherwise we have to guess.

// APIC IDs should always be unique; if not (false is returned), then
// something went wrong and the IDs shouldn't be used.
// side effect: sorts IDs and `removes' duplicates.
static bool AreApicIdsUnique(u8* apicIds, size_t numIds)
{
	std::sort(apicIds, apicIds+numIds);
	u8* const end = std::unique(apicIds, apicIds+numIds);
	const size_t numUnique = end-apicIds;
	// all unique => IDs are valid.
	if(numUnique == numIds)
		return true;

	// all zero => the system lacks an xAPIC.
	if(numUnique == 1 && apicIds[0] == 0)
		return false;

	// duplicated IDs => something went wrong. for example, VMs might not
	// expose all physical processors, and OS X still doesn't support
	// thread affinity masks.
	return false;
}

static u8 apicIdStorage[os_cpu_MaxProcessors];
static const u8* apicIds;	// = apicIdStorage, or 0 if IDs invalid

static Status InitApicIds()
{
	struct StoreEachProcessorsApicId
	{
		static void Callback(size_t processor, uintptr_t UNUSED(cbData))
		{
			apicIdStorage[processor] = x86_x64_ApicId();
		}
	};
	// (fails if the OS limits our process affinity)
	if(os_cpu_CallByEachCPU(StoreEachProcessorsApicId::Callback, (uintptr_t)&apicIds) == INFO::OK)
	{
		if(AreApicIdsUnique(apicIdStorage, os_cpu_NumProcessors()))
			apicIds = apicIdStorage;	// success, ApicIds will return this pointer
	}

	return INFO::OK;
}

const u8* ApicIds()
{
	static ModuleInitState initState;
	ModuleInit(&initState, InitApicIds);
	return apicIds;
}


size_t ProcessorFromApicId(size_t apicId)
{
	const u8* apicIds = ApicIds();
	const u8* end = apicIds + os_cpu_NumProcessors();
	const u8* pos = std::find(apicIds, end, apicId);
	if(pos == end)
	{
		ENSURE(0);
		return 0;
	}
	return pos - apicIds;	// index
}


struct ApicField	// POD
{
	size_t operator()(size_t bits) const
	{
		return (bits >> shift) & mask;
	}

	size_t mask;	// zero for zero-width fields
	size_t shift;
};


//-----------------------------------------------------------------------------
// CPU topology interface


struct CpuTopology	// POD
{
	ApicField logical;
	ApicField core;
	ApicField package;

	// how many are actually enabled
	size_t logicalPerCore;
	size_t coresPerPackage;
	size_t numPackages;
};
static CpuTopology cpuTopology;
static ModuleInitState cpuInitState;

static Status InitCpuTopology()
{
	const size_t maxLogicalPerCore = MaxLogicalPerCore();
	const size_t maxCoresPerPackage = MaxCoresPerPackage();
	const size_t maxPackages = 256;	// "enough"

	const size_t logicalWidth = ceil_log2(maxLogicalPerCore);
	const size_t coreWidth    = ceil_log2(maxCoresPerPackage);
	const size_t packageWidth = ceil_log2(maxPackages);

	cpuTopology.logical.mask = bit_mask<size_t>(logicalWidth);
	cpuTopology.core.mask    = bit_mask<size_t>(coreWidth);
	cpuTopology.package.mask = bit_mask<size_t>(packageWidth);

	cpuTopology.logical.shift = 0;
	cpuTopology.core.shift    = logicalWidth;
	cpuTopology.package.shift = logicalWidth + coreWidth;

	const u8* apicIds = ApicIds();
	if(apicIds)
	{
		struct NumUniqueValuesInField
		{
			size_t operator()(const u8* apicIds, const ApicField& apicField) const
			{
				std::set<size_t> values;
				for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
				{
					const size_t value = apicField(apicIds[processor]);
					values.insert(value);
				}
				return values.size();
			}
		};

		cpuTopology.logicalPerCore  = NumUniqueValuesInField()(apicIds, cpuTopology.logical);
		cpuTopology.coresPerPackage = NumUniqueValuesInField()(apicIds, cpuTopology.core);
		cpuTopology.numPackages     = NumUniqueValuesInField()(apicIds, cpuTopology.package);
	}
	else // the processor lacks an xAPIC, or the IDs are invalid
	{
		struct MinPackages
		{
			size_t operator()(size_t maxCoresPerPackage, size_t maxLogicalPerCore) const
			{
				const size_t numNodes = numa_NumNodes();
				const size_t logicalPerNode = PopulationCount(numa_ProcessorMaskFromNode(0));
				// NB: some cores or logical processors may be disabled.
				const size_t maxLogicalPerPackage = maxCoresPerPackage*maxLogicalPerCore;
				const size_t minPackagesPerNode = DivideRoundUp(logicalPerNode, maxLogicalPerPackage);
				return minPackagesPerNode*numNodes;
			}
		};

		// we can't differentiate between cores and logical processors.
		// since the former are less likely to be disabled, we seek the
		// maximum feasible number of cores and minimal number of packages:
		const size_t minPackages = MinPackages()(maxCoresPerPackage, maxLogicalPerCore);
		const size_t numProcessors = os_cpu_NumProcessors();
		for(size_t numPackages = minPackages; numPackages <= numProcessors; numPackages++)
		{
			if(numProcessors % numPackages != 0)
				continue;
			const size_t logicalPerPackage = numProcessors / numPackages;
			const size_t minCoresPerPackage = DivideRoundUp(logicalPerPackage, maxLogicalPerCore);
			for(size_t coresPerPackage = maxCoresPerPackage; coresPerPackage >= minCoresPerPackage; coresPerPackage--)
			{
				if(logicalPerPackage % coresPerPackage != 0)
					continue;
				const size_t logicalPerCore = logicalPerPackage / coresPerPackage;
				if(logicalPerCore <= maxLogicalPerCore)
				{
					ENSURE(numProcessors == numPackages*coresPerPackage*logicalPerCore);
					cpuTopology.logicalPerCore = logicalPerCore;
					cpuTopology.coresPerPackage = coresPerPackage;
					cpuTopology.numPackages = numPackages;
					return INFO::OK;
				}
			}
		}

		ENSURE(0);	// didn't find a feasible topology
	}

	return INFO::OK;
}

size_t cpu_topology_NumPackages()
{
	ModuleInit(&cpuInitState, InitCpuTopology);
	return cpuTopology.numPackages;
}

size_t cpu_topology_CoresPerPackage()
{
	ModuleInit(&cpuInitState, InitCpuTopology);
	return cpuTopology.coresPerPackage;
}

size_t cpu_topology_LogicalPerCore()
{
	ModuleInit(&cpuInitState, InitCpuTopology);
	return cpuTopology.logicalPerCore;
}

size_t cpu_topology_LogicalFromApicId(size_t apicId)
{
	ModuleInit(&cpuInitState, InitCpuTopology);
	return cpuTopology.logical(apicId);
}

size_t cpu_topology_CoreFromApicId(size_t apicId)
{
	ModuleInit(&cpuInitState, InitCpuTopology);
	return cpuTopology.core(apicId);
}

size_t cpu_topology_PackageFromApicId(size_t apicId)
{
	ModuleInit(&cpuInitState, InitCpuTopology);
	return cpuTopology.package(apicId);
}

size_t cpu_topology_ApicId(size_t idxLogical, size_t idxCore, size_t idxPackage)
{
	ModuleInit(&cpuInitState, InitCpuTopology);

	// NB: APIC IDs aren't guaranteed to be contiguous;
	// quad-core E5630 CPUs report 4-bit core IDs 0, 1, 6, 7.
	// we therefore compute an index into the sorted ApicIds array.

	size_t idx = 0;
	ENSURE(idxPackage < cpuTopology.numPackages);
	idx += idxPackage;

	idx *= cpuTopology.coresPerPackage;
	ENSURE(idxCore < cpuTopology.coresPerPackage);
	idx += idxCore;

	idx *= cpuTopology.logicalPerCore;
	ENSURE(idxLogical < cpuTopology.logicalPerCore);
	idx += idxLogical;

	ENSURE(idx < os_cpu_NumProcessors());
	const size_t apicId = ApicIds()[idx];
	return apicId;
}


//-----------------------------------------------------------------------------
// cache topology

// note: Windows 2003 GetLogicalProcessorInformation provides similar
// functionality but returns incorrect results. (it claims all cores in
// an Intel Core2 Quad processor share a single L2 cache.)

class CacheRelations
{
public:
	/**
	 * add processor to the processor mask owned by cache identified by \<id\>
	 **/
	void Add(u8 cacheId, size_t processor)
	{
		SharedCache* cache = Find(cacheId);
		if(!cache)
		{
			m_caches.push_back(cacheId);
			cache = &m_caches.back();
		}
		cache->Add(processor);
	}

	size_t NumCaches() const
	{
		return m_caches.size();
	}

	/**
	 * store topology in an array (one entry per cache) of masks
	 * representing the processors that share a cache.
	 **/
	void StoreProcessorMasks(uintptr_t* cachesProcessorMask)
	{
		for(size_t i = 0; i < NumCaches(); i++)
			cachesProcessorMask[i] = m_caches[i].ProcessorMask();
	}

private:
	/**
	 * stores ID and tracks which processors share this cache
	 **/
	class SharedCache
	{
	public:
		SharedCache(u8 cacheId)
			: m_cacheId(cacheId), m_processorMask(0)
		{
		}

		bool Matches(u8 id) const
		{
			return m_cacheId == id;
		}

		void Add(size_t processor)
		{
			m_processorMask |= uintptr_t(1) << processor;
		}

		uintptr_t ProcessorMask() const
		{
			return m_processorMask;
		}

	private:
		u8 m_cacheId;
		uintptr_t m_processorMask;
	};

	SharedCache* Find(u8 cacheId)
	{
		for(size_t i = 0; i < m_caches.size(); i++)
		{
			if(m_caches[i].Matches(cacheId))
				return &m_caches[i];
		}

		return 0;
	}

	std::vector<SharedCache> m_caches;
};

static void DetermineCachesProcessorMask(const u8* apicIds, uintptr_t* cachesProcessorMask, size_t& numCaches)
{
	CacheRelations cacheRelations;
	if(apicIds)
	{
		const size_t numBits = ceil_log2(MaxLogicalPerCache());
		const u8 cacheIdMask = u8((0xFF << numBits) & 0xFF);
		for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
		{
			const u8 apicId = apicIds[processor];
			const u8 cacheId = u8(apicId & cacheIdMask);
			cacheRelations.Add(cacheId, processor);
		}
	}
	else
	{
		for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
		{
			// assume each processor has exactly one cache with matching IDs
			const u8 cacheId = (u8)processor;
			cacheRelations.Add(cacheId, processor);
		}
	}

	numCaches = cacheRelations.NumCaches();
	cacheRelations.StoreProcessorMasks(cachesProcessorMask);
}


static void DetermineProcessorsCache(const uintptr_t* cachesProcessorMask, size_t numCaches, size_t* processorsCache, size_t numProcessors)
{
	for(size_t cache = 0; cache < numCaches; cache++)
	{
		// write to all entries that share this cache
		const uintptr_t processorMask = cachesProcessorMask[cache];
		for(size_t processor = 0; processor < numProcessors; processor++)
		{
			if(IsBitSet(processorMask, processor))
			{
				ENSURE(processorsCache[processor] == 0);
				processorsCache[processor] = cache;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// cache topology interface

struct CacheTopology	// POD
{
	size_t numCaches;
	size_t processorsCache[os_cpu_MaxProcessors];
	uintptr_t cachesProcessorMask[os_cpu_MaxProcessors];
};
static CacheTopology cacheTopology;
static ModuleInitState cacheInitState;

static Status InitCacheTopology()
{
	const u8* apicIds = ApicIds();
	DetermineCachesProcessorMask(apicIds, cacheTopology.cachesProcessorMask, cacheTopology.numCaches);
	DetermineProcessorsCache(cacheTopology.cachesProcessorMask, cacheTopology.numCaches, cacheTopology.processorsCache, os_cpu_NumProcessors());
	return INFO::OK;
}

size_t cache_topology_NumCaches()
{
	ModuleInit(&cacheInitState, InitCacheTopology);
	return cacheTopology.numCaches;
}

size_t cache_topology_CacheFromProcessor(size_t processor)
{
	ModuleInit(&cacheInitState, InitCacheTopology);
	ENSURE(processor < os_cpu_NumProcessors());
	return cacheTopology.processorsCache[processor];
}

uintptr_t cache_topology_ProcessorMaskFromCache(size_t cache)
{
	ModuleInit(&cacheInitState, InitCacheTopology);
	ENSURE(cache < cacheTopology.numCaches);
	return cacheTopology.cachesProcessorMask[cache];
}
