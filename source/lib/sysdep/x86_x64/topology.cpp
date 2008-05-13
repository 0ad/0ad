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
		logicalPerCache = 1;	// caches aren't shared unless we find a descriptor

		// note: Intel Appnote 485 says the order in which caches are returned is
		// undefined, so we need to loop through all of them.
		for(u32 count = 0; ; count++)
		{
			// get next cache descriptor
			x86_x64_CpuidRegs regs;
			regs.eax = 4;
			regs.ecx = count;
			x86_x64_cpuid(&regs);
			const u32 type = bits(regs.eax, 0, 4);
			if(type == 0)	// no more remaining
				break;
			
			struct IsL2DataCache
			{
				bool operator()(u32 type, u32 level) const
				{
					if(type != 1 && type != 3)	// neither data nor unified
						return false;
					if(level != 2)
						return false;
					return true;
				}
			};
			const u32 level = bits(regs.eax, 5, 7);
			if(IsL2DataCache()(type, level))
				logicalPerCache = bits(regs.eax, 14, 25)+1;
		}
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
 * no APIC is present or process affinity is limited.
 **/
static const u8* ApicIds()
{
	static u8 apicIdStorage[os_cpu_MaxProcessors];
	static const u8* apicIds;

	static volatile uintptr_t initialized = 0;
	if(cpu_CAS(&initialized, 0, 1))
	{
		// requires 'new' APIC (see x86_x64_ApicId for details)
		if(x86_x64_Generation() >= 8)
		{
			// store each processor's APIC ID in turn
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
 * count the number of unique values assumed by a certain field (i.e. part
 * of the APIC ID).
 * @param numBits width of the field; must be set to ceil_log2 of the
 * maximum value that can be assumed by the field.
 * @return number of unique values (one if numBits is zero - this is
 * convenient and kind of justified by counting the empty symbol)
 **/
static size_t NumUniqueValuesInField(const u8* apicIds, size_t offset, size_t numBits)
{
	if(numBits == 0)
		return 1;	// see above
	const u8 mask = bit_mask<u8>(numBits);

	typedef std::set<u8> IdSet;
	IdSet ids;
	for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
	{
		const u8 apicId = apicIds[processor];
		const u8 field = u8(apicId >> offset) & mask;
		ids.insert(field);
	}

	return ids.size();
}


size_t cpu_NumPackages()
{
	static size_t numPackages = 0;

	if(!numPackages)
	{
		const u8* apicIds = ApicIds();
		if(apicIds)
		{
			const size_t offset = ceil_log2(CoresPerPackage()) + ceil_log2(LogicalPerCore());
			const size_t numBits = 8;
			numPackages = NumUniqueValuesInField(apicIds, offset, numBits);
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
			// assume hyperthreads are enabled; check if they count as processors.
			if(numPackagesTimesLogical > LogicalPerCore())
				numPackages = numPackagesTimesLogical / LogicalPerCore();
		}
	}

	return numPackages;
}


size_t cpu_CoresPerPackage()
{
	static size_t enabledCoresPerPackage;

	if(!enabledCoresPerPackage)
	{
		const u8* apicIds = ApicIds();
		if(apicIds)
		{
			const size_t offset = ceil_log2(LogicalPerCore());
			const size_t numBits = ceil_log2(CoresPerPackage());
			enabledCoresPerPackage = NumUniqueValuesInField(apicIds, offset, numBits);
		}
		else
		{
			// guess (must match cpu_NumPackages's assumptions)
			enabledCoresPerPackage = CoresPerPackage();
		}
	}

	return enabledCoresPerPackage;
}


size_t cpu_LogicalPerCore()
{
	static size_t enabledLogicalPerCore;

	if(!enabledLogicalPerCore)
	{
		const u8* apicIds = ApicIds();
		if(apicIds)
		{
			const size_t offset = 0;
			const size_t numBits = ceil_log2(LogicalPerCore());
			enabledLogicalPerCore = NumUniqueValuesInField(apicIds, offset, numBits);
		}
		else
		{
			// guess (must match cpu_NumPackages's assumptions)
			enabledLogicalPerCore = LogicalPerCore();
		}
	}

	return enabledLogicalPerCore;
}


//-----------------------------------------------------------------------------
// cache topology

// note: Windows 2003 GetLogicalProcessorInformation provides similar
// functionality but returns incorrect results. (it claims all cores in
// an Intel Core2 Quad processor share a single L2 cache.)

size_t cpu_NumCaches()
{
	static size_t numCaches;
	if(!numCaches)
	{
		const u8* apicIds = ApicIds();
		if(apicIds)
		{
			const size_t offset = 0;
			const size_t numBits = ceil_log2(LogicalPerCache());
			numCaches = NumUniqueValuesInField(apicIds, offset, numBits);
		}
		else
		{
			// assume each processor has its own cache
			numCaches = os_cpu_NumProcessors();
		}
	}

	return numCaches;
}

class CacheTopology
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

uintptr_t cpu_ProcessorMaskFromCache(size_t cache)
{
	static uintptr_t cachesProcessorMask[os_cpu_MaxProcessors];

	static volatile uintptr_t initialized = 0;
	if(cpu_CAS(&initialized, 0, 1))
	{
		const u8* apicIds = ApicIds();
		if(apicIds)
		{
			const size_t numBits = ceil_log2(LogicalPerCache());
			const u8 cacheIdMask = u8(0xFF << numBits);

			CacheTopology cacheManager;
			for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
			{
				const u8 apicId = apicIds[processor];
				const u8 cacheId = apicId & cacheIdMask;
				cacheManager.Add(cacheId, processor);
			}
			cacheManager.StoreProcessorMasks(cachesProcessorMask);
		}
		else
		{
			// assume each cache belongs to exactly one processor and
			// cache index == processor index.
			for(size_t cache = 0; cache < cpu_NumCaches(); cache++)
				cachesProcessorMask[cache] = uintptr_t(1) << cache;
		}
	}

	debug_assert(cache < cpu_NumCaches());
	return cachesProcessorMask[cache];
}


size_t cpu_CacheFromProcessor(size_t processor)
{
	static size_t processorsCache[os_cpu_MaxProcessors];

	static volatile uintptr_t initialized = 0;
	if(cpu_CAS(&initialized, 0, 1))
	{
		for(size_t cache = 0; cache < cpu_NumCaches(); cache++)
		{
			// write to all entries that share this cache
			const uintptr_t processorMask = cpu_ProcessorMaskFromCache(cache);
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

	debug_assert(processor < os_cpu_NumProcessors());
	return processorsCache[processor];
}
