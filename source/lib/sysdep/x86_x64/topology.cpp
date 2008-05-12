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
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/os_cpu.h"
#include "x86_x64.h"


//-----------------------------------------------------------------------------

// note: Intel Appnote 485 (CPUID) assures uniformity of coresPerPackage and
// logicalPerCore across all packages.

static size_t DetectCoresPerPackage()
{
	x86_x64_CpuidRegs regs;
	switch(x86_x64_Vendor())
	{
	case X86_X64_VENDOR_INTEL:
		regs.eax = 4;
		regs.ecx = 0;
		if(x86_x64_cpuid(&regs))
			return bits(regs.eax, 26, 31)+1;
		break;

	case X86_X64_VENDOR_AMD:
		regs.eax = 0x80000008;
		if(x86_x64_cpuid(&regs))
			return bits(regs.ecx, 0, 7)+1;
		break;
	}

	return 1;	// else: the CPU is single-core.
}

static size_t CoresPerPackage()
{
	static size_t coresPerPackage = 0;
	if(!coresPerPackage)
		coresPerPackage = DetectCoresPerPackage();
	return coresPerPackage;
}


static bool IsHyperthreadingCapable()
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

static size_t DetectLogicalPerCore()
{
	if(!IsHyperthreadingCapable())
		return 1;

	x86_x64_CpuidRegs regs;
	regs.eax = 1;
	if(!x86_x64_cpuid(&regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
	const size_t logicalPerPackage = bits(regs.ebx, 16, 23);

	// cores ought to be uniform WRT # logical processors
	debug_assert(logicalPerPackage % CoresPerPackage() == 0);

	return logicalPerPackage / CoresPerPackage();
}

static size_t LogicalPerCore()
{
	static size_t logicalPerCore = 0;
	if(!logicalPerCore)
		logicalPerCore = DetectLogicalPerCore();
	return logicalPerCore;
}

enum CacheType
{
	CT_NONE = 0,
	CT_DATA = 1,
	CT_INSTRUCTION = 2,
	CT_UNIFIED = 3
};

static bool IsL2DataCache(CacheType type, size_t level)
{
	if(type != CT_DATA && type != CT_UNIFIED)
		return false;
	if(level != 2)
		return false;
	return true;
}

static size_t DetectLogicalPerCache()
{
	// note: Intel Appnote 485 says the order in which caches are returned is
	// undefined, so we need to loop through all of them.
	for(u32 count = 0; ; count++)
	{
		x86_x64_CpuidRegs regs;
		regs.eax = 4;
		regs.ecx = count;
		x86_x64_cpuid(&regs);
	
		const CacheType type = (CacheType)bits(regs.eax, 0, 4);
		// no more caches left
		if(type == CT_NONE)
		{
			debug_assert(0);	// we somehow didn't find the L2d
			return 1;
		}

		const size_t level = bits(regs.eax, 5, 7);
		if(IsL2DataCache(type, level))
		{
			const size_t logicalPerCache = bits(regs.eax, 14, 25)+1;
			return logicalPerCache;
		}
	}
}

static size_t LogicalPerCache()
{
	static size_t logicalPerCache;
	if(!logicalPerCache)
		logicalPerCache = DetectLogicalPerCache();
	return logicalPerCache;
}


//-----------------------------------------------------------------------------

// the above functions give the maximum number of cores/logical units.
// however, some of them may actually be disabled by the BIOS!
// what we can do is to analyze the APIC IDs. they are allocated sequentially
// for all "processors". treating the IDs as variable-width bit fields
// (according to the number of cores/logical units present) allows
// determining the exact topology as well as number of packages.

// these are set by DetectProcessorTopology.
static size_t numPackages = 0;	// i.e. sockets; > 1 => true SMP system
static size_t enabledCoresPerPackage = 0;
static size_t enabledLogicalPerCore = 0;	// hyperthreading units

typedef std::vector<u8> Ids;

// add the currently running processor's APIC ID to a list of IDs.
static void StoreApicId(size_t UNUSED(processor), uintptr_t cbData)
{
	Ids* const apicIds = (Ids*)cbData;
	apicIds->push_back(x86_x64_ApicId());
}

// if successful, apicIds[i] contains the unique ID of OS processor i.
static bool GatherApicIds(Ids& apicIds)
{
	// old APIC (see x86_x64_ApicId for details)
	if(x86_x64_Generation() < 8)
		return false;

	// process affinity prevents us from seeing all APIC IDs
	if(PopulationCount(os_cpu_ProcessorMask()) != os_cpu_NumProcessors())
		return false;

	const LibError ret = os_cpu_CallByEachCPU(StoreApicId, (uintptr_t)&apicIds);
	debug_assert(ret == INFO::OK);

	// ensure we got a unique ID for every processor
	{
		Ids tmp(apicIds);
		Ids::iterator end = tmp.end();
		std::sort(tmp.begin(), end);
		debug_assert(std::unique(tmp.begin(), end) == end);
		debug_assert(std::distance(tmp.begin(), end) == (ptrdiff_t)os_cpu_NumProcessors());
	}

	return true;
}


typedef std::set<u8> IdSet;

/**
 * "field" := a range of bits sufficient to represent <numValues> integers.
 * for each id in <apicIds>: extract the value of the field starting at
 * <offset> and insert it into <ids>. afterwards, adjust <offset> to the
 * next field.
 *
 * used to gather e.g. all core IDs from all APIC IDs.
 **/
static void ExtractFieldIntoSet(const Ids& apicIds, size_t& offset, size_t numValues, IdSet& ids)
{
	const size_t numBits = ceil_log2(numValues);
	if(numBits == 0)
		return;
	const u8 mask = bit_mask<u8>(numBits);

	for(size_t i = 0; i < apicIds.size(); i++)
	{
		const u8 apicId = apicIds[i];
		const u8 field = u8(apicId >> offset) & mask;
		ids.insert(field);
	}

	offset += numBits;
}

static size_t numCaches = 0;	// L2d
static std::vector<size_t> processorsCache;
static std::vector<uintptr_t> cachesProcessorMask;



class CacheManager
{
public:
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

	void StoreProcessorMasks(std::vector<uintptr_t>& processorMasks)
	{
		processorMasks.resize(m_caches.size());
		for(size_t i = 0; i < m_caches.size(); i++)
			processorMasks[i] = m_caches[i].ProcessorMask();
	}

private:
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

static void DetectCacheTopology(const Ids& apicIds)
{
	const size_t numBits = ceil_log2(LogicalPerCache());
	const u8 cacheIdMask = u8(0xFF << numBits);

	CacheManager cacheManager;
	for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
	{
		const u8 apicId = apicIds[processor];
		const u8 cacheId = apicId & cacheIdMask;
		cacheManager.Add(cacheId, processor);
	}
	cacheManager.StoreProcessorMasks(cachesProcessorMask);
	numCaches = cachesProcessorMask.size();

	const size_t invalidCache = ~(size_t)0;
	processorsCache.resize(os_cpu_NumProcessors(), invalidCache);
	for(size_t cache = 0; cache < numCaches; cache++)
	{
		const uintptr_t processorMask = cachesProcessorMask[cache];
		for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
		{
			if(IsBitSet(processorMask, processor))
				processorsCache[processor] = cache;
		}
	}
	for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
	{
		debug_assert(processorsCache[processor] != invalidCache);
		debug_assert(processorsCache[processor] < numCaches);
	}
}


// @return false if unavailable / no information can be returned.
static bool DetectProcessorTopologyViaApicIds()
{
	Ids apicIds;
	if(!GatherApicIds(apicIds))
		return false;

	// extract values from all 3 ID bit fields into separate sets
	size_t offset = 0;
	IdSet logicalIds;
	ExtractFieldIntoSet(apicIds, offset, LogicalPerCore(), logicalIds);
	IdSet coreIds;
	ExtractFieldIntoSet(apicIds, offset, CoresPerPackage(), coreIds);
	IdSet packageIds;
	ExtractFieldIntoSet(apicIds, offset, 0xFF, packageIds);

	numPackages            = std::max(packageIds.size(), size_t(1));
	enabledCoresPerPackage = std::max(coreIds   .size(), size_t(1));
	enabledLogicalPerCore  = std::max(logicalIds.size(), size_t(1));

	// note: cache ID possibly overlaps the other fields. we also want to
	// retrieve more information (mappings between processor and cache ID),
	// so this needs to be handled separately.
	DetectCacheTopology(apicIds);

	return true;
}


static void GuessProcessorTopologyViaOsCount()
{
	const size_t numProcessors = os_cpu_NumProcessors();

	// note: we cannot hope to always return correct results since disabled
	// cores/logical units cannot be distinguished from the situation of the
	// OS simply not reporting them as "processors". unfortunately this
	// function won't always only be called for older (#core = #logical = 1)
	// systems because DetectProcessorTopologyViaApicIds may fail due to
	// lack of OS support. what we'll do is assume nothing is disabled; this
	// is reasonable because we care most about #packages. it's fine to assume
	// more cores (without inflating the total #processors) because that
	// count only indicates memory barriers etc. ought to be used.
	enabledCoresPerPackage = CoresPerPackage();
	enabledLogicalPerCore = LogicalPerCore();

	const size_t numPackagesTimesLogical = numProcessors / CoresPerPackage();
	debug_assert(numPackagesTimesLogical != 0);	// otherwise processors didn't include cores, which would be stupid

	numPackages = numPackagesTimesLogical / LogicalPerCore();
	if(!numPackages)	// processors didn't include logical units (reasonable)
		numPackages = numPackagesTimesLogical;
}


// determine how many CoresPerPackage and LogicalPerCore are
// actually enabled and also count numPackages.
static void DetectProcessorTopology()
{
	// authoritative, but requires OS support and fairly recent CPUs
	if(DetectProcessorTopologyViaApicIds())
		return;	// success, we're done.

	GuessProcessorTopologyViaOsCount();
}


size_t cpu_NumPackages()
{
	if(!numPackages)
		DetectProcessorTopology();
	return numPackages;
}

size_t cpu_CoresPerPackage()
{
	if(!enabledCoresPerPackage)
		DetectProcessorTopology();
	return enabledCoresPerPackage;
}

size_t cpu_LogicalPerCore()
{
	if(!enabledLogicalPerCore)
		DetectProcessorTopology();
	return enabledLogicalPerCore;
}

size_t cpu_NumCaches()
{
	if(!numCaches)
		DetectProcessorTopology();
	return numCaches;
}


size_t cpu_CacheFromProcessor(size_t processor)
{
	debug_assert(processor < os_cpu_NumProcessors());
	DetectProcessorTopology();
	return processorsCache.at(processor);
}

uintptr_t cpu_ProcessorMaskFromCache(size_t cache)
{
	debug_assert(cache < cpu_NumCaches());
	DetectProcessorTopology();
	return cachesProcessorMask.at(cache);
}


// note: Windows 2003 GetLogicalProcessorInformation returns incorrect
// information, claiming all cores in an Intel Core2 Quad processor
// share an L2 cache.
