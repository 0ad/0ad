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
		debug_assert(logicalPerPackage % maxCoresPerPackage == 0);
		const size_t maxLogicalPerCore = logicalPerPackage / maxCoresPerPackage;
		return maxLogicalPerCore;
	}
	else
		return 1;
}


static size_t MaxLogicalPerCache()
{
	const x86_x64_Caches* const dcaches = x86_x64_DCaches();
	if(dcaches->numLevels >= 2)
		return dcaches->levels[1].sharedBy;
	else
		return 1;	// default
}


//-----------------------------------------------------------------------------
// determination of enabled cores/HTs

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

static LibError InitApicIds()
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


/**
 * count the number of unique APIC IDs after application of a mask.
 *
 * this is used to implement NumUniqueValuesInField and also required
 * for counting the number of caches.
 **/
static size_t NumUniqueMaskedValues(const u8* apicIds, u8 mask)
{
	std::set<u8> ids;
	for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
	{
		const u8 apicId = apicIds[processor];
		const u8 field = u8(apicId & mask);
		ids.insert(field);
	}

	return ids.size();
}


/**
 * Count the number of values assumed by a certain field within APIC IDs.
 *
 * @param apicIds
 * @param offset Index of the lowest bit that is part of the field.
 * @param numValues Number of values that can be assumed by the field.
 *		  If equal to one, the field is zero-width.
 * @return number of unique values (for convenience of the topology code,
 * this is always at least one)
 **/
static size_t NumUniqueValuesInField(const u8* apicIds, size_t offset, size_t numValues)
{
	if(numValues == 1)	// see parameter description above
		return 1;
	const size_t numBits = ceil_log2(numValues);
	const u8 mask = u8((bit_mask<u8>(numBits) << offset) & 0xFF);
	return NumUniqueMaskedValues(apicIds, mask);
}


static size_t MinPackages(size_t maxCoresPerPackage, size_t maxLogicalPerCore)
{
	const size_t numNodes = numa_NumNodes();
	const size_t logicalPerNode = PopulationCount(numa_ProcessorMaskFromNode(0));
	// NB: some cores or logical processors may be disabled.
	const size_t maxLogicalPerPackage = maxCoresPerPackage*maxLogicalPerCore;
	const size_t minPackagesPerNode = DivideRoundUp(logicalPerNode, maxLogicalPerPackage);
	return minPackagesPerNode*numNodes;
}


struct CpuTopology	// POD
{
	size_t numPackages;
	size_t coresPerPackage;
	size_t logicalPerCore;
};
static CpuTopology cpuTopology;
static ModuleInitState cpuInitState;

static LibError InitCpuTopology()
{
	const size_t numProcessors = os_cpu_NumProcessors();
	const size_t maxCoresPerPackage = MaxCoresPerPackage();
	const size_t maxLogicalPerCore = MaxLogicalPerCore();

	const u8* apicIds = ApicIds();
	if(apicIds)
	{
		const size_t packageOffset = ceil_log2(maxCoresPerPackage) + ceil_log2(maxLogicalPerCore);
		const size_t coreOffset    = ceil_log2(maxLogicalPerCore);
		const size_t logicalOffset = 0;
		cpuTopology.numPackages     = NumUniqueValuesInField(apicIds, packageOffset, 256);
		cpuTopology.coresPerPackage = NumUniqueValuesInField(apicIds, coreOffset,    maxCoresPerPackage);
		cpuTopology.logicalPerCore  = NumUniqueValuesInField(apicIds, logicalOffset, maxLogicalPerCore);
	}
	else // the processor lacks an xAPIC, or the IDs are invalid
	{
		// we can't differentiate between cores and logical processors.
		// since the former are less likely to be disabled, we seek the
		// maximum feasible number of cores and minimal number of packages:
		const size_t minPackages = MinPackages(maxCoresPerPackage, maxLogicalPerCore);
		const size_t maxPackages = numProcessors;
		for(size_t numPackages = minPackages; numPackages <= maxPackages; numPackages++)
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
					debug_assert(numProcessors == numPackages*coresPerPackage*logicalPerCore);
					cpuTopology.numPackages = numPackages;
					cpuTopology.coresPerPackage = coresPerPackage;
					cpuTopology.logicalPerCore = logicalPerCore;
					return INFO::OK;
				}
			}
		}

		debug_assert(0);	// didn't find a feasible topology
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
				debug_assert(processorsCache[processor] == 0);
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

static LibError InitCacheTopology()
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
	debug_assert(processor < os_cpu_NumProcessors());
	return cacheTopology.processorsCache[processor];
}

uintptr_t cache_topology_ProcessorMaskFromCache(size_t cache)
{
	ModuleInit(&cacheInitState, InitCacheTopology);
	debug_assert(cache < cacheTopology.numCaches);
	return cacheTopology.cachesProcessorMask[cache];
}
