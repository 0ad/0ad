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

/**
 * =========================================================================
 * File        : topology.cpp
 * Project     : 0 A.D.
 * Description : detection of CPU and cache topology
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "topology.h"

#include "lib/bits.h"
#include "lib/sysdep/cpu.h"	// ERR::CPU_FEATURE_MISSING
#include "lib/sysdep/os_cpu.h"
#include "x86_x64.h"


//-----------------------------------------------------------------------------
// detect *maximum* number of cores/packages/caches.
// note: some of them may be disabled by the OS or BIOS.
// note: Intel Appnote 485 assures us that they are uniform across packages.

static size_t CoresPerPackage()
{
	static size_t coresPerPackage = 0;

	if(!coresPerPackage)
	{
		coresPerPackage = 1;	// it's single core unless one of the following applies:

		x86_x64_CpuidRegs regs;
		switch(x86_x64_Vendor())
		{
		case X86_X64_VENDOR_INTEL:
			regs.eax = 4;
			regs.ecx = 0;
			if(x86_x64_cpuid(&regs))
				coresPerPackage = bits(regs.eax, 26, 31)+1;
			break;

		case X86_X64_VENDOR_AMD:
			regs.eax = 0x80000008;
			if(x86_x64_cpuid(&regs))
				coresPerPackage = bits(regs.ecx, 0, 7)+1;
			break;
		}
	}

	return coresPerPackage;
}


static size_t LogicalPerCore()
{
	static size_t logicalPerCore = 0;

	if(!logicalPerCore)
	{
		struct IsHyperthreadingCapable
		{
			bool operator()() const
			{
				// definitely not
				if(!x86_x64_cap(X86_X64_CAP_HT))
					return false;

				// AMD N-core systems falsely set the HT bit for compatibility reasons
				// (don't bother resetting it, might confuse callers)
				if(x86_x64_Vendor() == X86_X64_VENDOR_AMD && x86_x64_cap(X86_X64_CAP_AMD_CMP_LEGACY))
					return false;

				return true;
			}
		};
		if(!IsHyperthreadingCapable()())
			logicalPerCore = 1;
		else
		{
			x86_x64_CpuidRegs regs;
			regs.eax = 1;
			if(!x86_x64_cpuid(&regs))
				DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
			const size_t logicalPerPackage = bits(regs.ebx, 16, 23);
			// cores ought to be uniform WRT # logical processors
			debug_assert(logicalPerPackage % CoresPerPackage() == 0);
			logicalPerCore = logicalPerPackage / CoresPerPackage();
		}
	}

	return logicalPerCore;
}


static size_t LogicalPerCache()
{
	static size_t logicalPerCache;

	if(!logicalPerCache)
	{
		const x86_x64_Cache* const dcache = x86_x64_DCache();
		if(dcache->levels < 2)
			logicalPerCache = 1;	// default
		else
			logicalPerCache = dcache->parameters[1].sharedBy;
	}

	return logicalPerCache;
}


//-----------------------------------------------------------------------------
// determination of enabled cores/HTs

// APIC IDs consist of variable-length fields identifying the logical unit,
// core, package and shared cache. if they are available, we can determine
// the exact topology; otherwise we have to guess.

/**
 * @return an array of the processors' unique APIC IDs or zero if
 * no xAPIC is present or process affinity is limited.
 **/
static const u8* ApicIds()
{
	const u8* const uninitialized = (const u8*)1;
	static const u8* apicIds = uninitialized;

	if(apicIds == uninitialized)
	{
		apicIds = 0;	// return zero from now on unless the below succeeds

		// requires xAPIC (see x86_x64_ApicId for details)
		if(x86_x64_Generation() >= 8)
		{
			// store each processor's APIC ID in turn
			static u8 apicIdStorage[os_cpu_MaxProcessors];
			struct StoreApicId
			{
				static void Callback(size_t processor, uintptr_t UNUSED(cbData))
				{
					apicIdStorage[processor] = x86_x64_ApicId();
				}
			};
			if(os_cpu_CallByEachCPU(StoreApicId::Callback, (uintptr_t)&apicIds) == INFO::OK)
				apicIds = apicIdStorage;	// success, return valid array from now on
		}
	}

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
		const u8 field = apicId & mask;
		ids.insert(field);
	}

	return ids.size();
}


/**
 * count the number of values assumed by a certain field within APIC IDs.
 *
 * @param offset index of the lowest bit that is part of the field.
 * @param numValues number of values that can be assumed by the field.
 * if equal to one, the field is zero-width.
 * @return number of unique values (for convenience of the topology code,
 * this is always at least one)
 **/
static size_t NumUniqueValuesInField(const u8* apicIds, size_t offset, size_t numValues)
{
	if(numValues == 1)
		return 1;	// see above
	const size_t numBits = ceil_log2(numValues);
	const u8 mask = u8((bit_mask<u8>(numBits) << offset) & 0xFF);
	return NumUniqueMaskedValues(apicIds, mask);
}


static size_t NumPackages(const u8* apicIds)
{
	if(apicIds)
	{
		const size_t offset = ceil_log2(CoresPerPackage()) + ceil_log2(LogicalPerCore());
		return NumUniqueValuesInField(apicIds, offset, 256);
	}
	else
	{
		// note: correct results cannot be guaranteed because unreported
		// and disable logical units are indistinguishable. the below
		// assumptions are reasonable because we care most about packages
		// (i.e. whether the system is truly SMP). in contrast, it is
		// safe to overestimate the number of cores because that
		// only determines if memory barriers are needed or not.
		// note: requiring modern processors featuring an APIC does not
		// prevent this from being reached (the cause may be lack of
		// OS support or restricted process affinity).

		// assume cores are enabled and count as processors.
		const size_t numPackagesTimesLogical = os_cpu_NumProcessors() / CoresPerPackage();
		debug_assert(numPackagesTimesLogical != 0);
		// assume hyperthreads are enabled.
		size_t numPackages = numPackagesTimesLogical;
		// if they are reported as processors, remove them from the count.
		if(numPackages > LogicalPerCore())
			numPackages /= LogicalPerCore();
		return numPackages;
	}
}


static size_t CoresPerPackage(const u8* apicIds)
{
	if(apicIds)
	{
		const size_t offset = ceil_log2(LogicalPerCore());
		return NumUniqueValuesInField(apicIds, offset, CoresPerPackage());
	}
	else
	{
		// guess (must match NumPackages's assumptions)
		return CoresPerPackage();
	}
}


static size_t LogicalPerCore(const u8* apicIds)
{
	if(apicIds)
	{
		const size_t offset = 0;
		return NumUniqueValuesInField(apicIds, offset, LogicalPerCore());
	}
	else
	{
		// guess (must match NumPackages's assumptions)
		return LogicalPerCore();
	}
}


//-----------------------------------------------------------------------------
// CPU topology interface

struct CpuTopology	// POD
{
	size_t numPackages;
	size_t coresPerPackage;
	size_t logicalPerCore;
};

const CpuTopology* cpu_topology_Detect()
{
	static CpuTopology topology;

	if(!topology.numPackages)
	{
		const u8* apicIds = ApicIds();
		topology.numPackages = NumPackages(apicIds);
		topology.coresPerPackage = CoresPerPackage(apicIds);
		topology.logicalPerCore = LogicalPerCore(apicIds);
	}

	return &topology;
}

size_t cpu_topology_NumPackages(const CpuTopology* topology)
{
	return topology->numPackages;
}

size_t cpu_topology_CoresPerPackage(const CpuTopology* topology)
{
	return topology->coresPerPackage;
}

size_t cpu_topology_LogicalPerCore(const CpuTopology* topology)
{
	return topology->logicalPerCore;
}


//-----------------------------------------------------------------------------
// cache topology

// note: Windows 2003 GetLogicalProcessorInformation provides similar
// functionality but returns incorrect results. (it claims all cores in
// an Intel Core2 Quad processor share a single L2 cache.)

static size_t NumCaches(const u8* apicIds)
{
	if(apicIds)
	{
		const size_t numBits = ceil_log2(LogicalPerCache());
		const u8 mask = u8((0xFF << numBits) & 0xFF);
		return NumUniqueMaskedValues(apicIds, mask);
	}
	else
	{
		// assume each processor has its own cache
		return os_cpu_NumProcessors();
	}
}

class CacheRelations
{
public:
	/**
	 * add processor to the processor mask owned by cache identified by <id>
	 **/
	void Add(u8 id, size_t processor)
	{
		SharedCache* cache = Find(id);
		if(!cache)
		{
			m_caches.push_back(id);
			cache = &m_caches.back();
		}
		cache->Add(processor);
	}

	/**
	 * store topology in an array (one entry per cache) of masks
	 * representing the processors that share a cache.
	 **/
	void StoreProcessorMasks(uintptr_t* processorMasks)
	{
		for(size_t i = 0; i < m_caches.size(); i++)
			processorMasks[i] = m_caches[i].ProcessorMask();
	}

private:
	/**
	 * stores ID and tracks which processors share this cache
	 **/
	class SharedCache
	{
	public:
		SharedCache(u8 id)
			: m_id(id), m_processorMask(0)
		{
		}

		bool Matches(u8 id) const
		{
			return m_id == id;
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
		u8 m_id;
		uintptr_t m_processorMask;
	};

	SharedCache* Find(u8 id)
	{
		for(size_t i = 0; i < m_caches.size(); i++)
		{
			if(m_caches[i].Matches(id))
				return &m_caches[i];
		}

		return 0;
	}

	std::vector<SharedCache> m_caches;
};

static void DetermineCachesProcessorMask(const u8* apicIds, uintptr_t* cachesProcessorMask)
{
	if(apicIds)
	{
		const size_t numBits = ceil_log2(LogicalPerCache());
		const u8 cacheIdMask = u8(0xFF << numBits);

		CacheRelations cacheRelations;
		for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
		{
			const u8 apicId = apicIds[processor];
			const u8 cacheId = apicId & cacheIdMask;
			cacheRelations.Add(cacheId, processor);
		}
		cacheRelations.StoreProcessorMasks(cachesProcessorMask);
	}
	else
	{
		// assume each processor has exactly one cache with matching IDs
		for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
			cachesProcessorMask[processor] = uintptr_t(1) << processor;
	}
}


static void DetermineProcessorsCache(size_t numCaches, const uintptr_t* cachesProcessorMask, size_t* processorsCache)
{
	for(size_t cache = 0; cache < numCaches; cache++)
	{
		// write to all entries that share this cache
		const uintptr_t processorMask = cachesProcessorMask[cache];
		for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
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

const CacheTopology* cache_topology_Detect()
{
	static CacheTopology topology;

	if(!topology.numCaches)
	{
		const u8* apicIds = ApicIds();
		topology.numCaches = NumCaches(apicIds);
		DetermineCachesProcessorMask(apicIds, topology.cachesProcessorMask);
		DetermineProcessorsCache(topology.numCaches, topology.cachesProcessorMask, topology.processorsCache);
	}

	return &topology;
}

size_t cache_topology_NumCaches(const CacheTopology* topology)
{
	return topology->numCaches;
}

size_t cache_topology_CacheFromProcessor(const CacheTopology* topology, size_t processor)
{
	debug_assert(processor < os_cpu_NumProcessors());
	return topology->processorsCache[processor];
}

uintptr_t cache_topology_ProcessorMaskFromCache(const CacheTopology* topology, size_t cache)
{
	debug_assert(cache < topology->numCaches);
	return topology->cachesProcessorMask[cache];
}
