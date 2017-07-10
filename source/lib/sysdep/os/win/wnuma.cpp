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

#include "precompiled.h"
#include "lib/sysdep/numa.h"

#include "lib/bits.h"	// PopulationCount
#include "lib/alignment.h"
#include "lib/timer.h"
#include "lib/module_init.h"
#include "lib/sysdep/vm.h"
#include "lib/sysdep/acpi.h"
#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/wutil.h"
#include "lib/sysdep/os/win/wcpu.h"
#include <Psapi.h>

#if ARCH_X86_X64
#include "lib/sysdep/arch/x86_x64/apic.h"	// ProcessorFromApicId
#endif


//-----------------------------------------------------------------------------
// nodes

struct Node	// POD
{
	// (Windows doesn't guarantee node numbers are contiguous, so
	// we associate them with contiguous indices in nodes[])
	UCHAR nodeNumber;

	u32 proximityDomainNumber;
	uintptr_t processorMask;
};

static Node nodes[os_cpu_MaxProcessors];
static size_t numNodes;

static Node* AddNode()
{
	ENSURE(numNodes < ARRAY_SIZE(nodes));
	return &nodes[numNodes++];
}

static Node* FindNodeWithProcessorMask(uintptr_t processorMask)
{
	for(size_t node = 0; node < numNodes; node++)
	{
		if(nodes[node].processorMask == processorMask)
			return &nodes[node];
	}

	return 0;
}

static Node* FindNodeWithProcessor(size_t processor)
{
	for(size_t node = 0; node < numNodes; node++)
	{
		if(IsBitSet(nodes[node].processorMask, processor))
			return &nodes[node];
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Windows topology

static UCHAR HighestNodeNumber()
{
	WUTIL_FUNC(pGetNumaHighestNodeNumber, BOOL, (PULONG));
	WUTIL_IMPORT_KERNEL32(GetNumaHighestNodeNumber, pGetNumaHighestNodeNumber);
	if(!pGetNumaHighestNodeNumber)
		return 0;	// NUMA not supported => only one node

	ULONG highestNodeNumber;
	const BOOL ok = pGetNumaHighestNodeNumber(&highestNodeNumber);
	WARN_IF_FALSE(ok);
	return (UCHAR)highestNodeNumber;
}

static void PopulateNodes()
{
	WUTIL_FUNC(pGetNumaNodeProcessorMask, BOOL, (UCHAR, PULONGLONG));
	WUTIL_IMPORT_KERNEL32(GetNumaNodeProcessorMask, pGetNumaNodeProcessorMask);
	if(!pGetNumaNodeProcessorMask)
		return;

	DWORD_PTR processAffinity, systemAffinity;
	{
		const BOOL ok = GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity);
		WARN_IF_FALSE(ok);
	}
	ENSURE(PopulationCount(processAffinity) <= PopulationCount(systemAffinity));

	for(UCHAR nodeNumber = 0; nodeNumber <= HighestNodeNumber(); nodeNumber++)
	{
		ULONGLONG affinity;
		{
			const BOOL ok = pGetNumaNodeProcessorMask(nodeNumber, &affinity);
			WARN_IF_FALSE(ok);
		}
		if(!affinity)
			continue;	// empty node, skip

		Node* node = AddNode();
		node->nodeNumber = nodeNumber;
		node->processorMask = wcpu_ProcessorMaskFromAffinity(processAffinity, (DWORD_PTR)affinity);
	}
}


//-----------------------------------------------------------------------------
// ACPI SRAT topology

#if ARCH_X86_X64

#pragma pack(push, 1)

// fields common to Affinity* structures
struct AffinityHeader
{
	u8 type;
	u8 length;	// size [bytes], including this header
};

struct AffinityAPIC
{
	static const u8 type = 0;

	AffinityHeader header;
	u8 proximityDomainNumber0;
	u8 apicId;
	u32 flags;
	u8 sapicId;
	u8 proximityDomainNumber123[3];
	u32 clockDomain;

	u32 ProximityDomainNumber() const
	{
		// (this is the apparent result of backwards compatibility, ugh.)
		u32 proximityDomainNumber;
		memcpy(&proximityDomainNumber, &proximityDomainNumber123[0]-1, sizeof(proximityDomainNumber));
		proximityDomainNumber &= ~0xFF;
		proximityDomainNumber |= proximityDomainNumber0;
		return proximityDomainNumber;
	}
};

struct AffinityMemory
{
	static const u8 type = 1;

	AffinityHeader header;
	u32 proximityDomainNumber;
	u16 reserved1;
	u64 baseAddress;
	u64 length;
	u32 reserved2;
	u32 flags;
	u64 reserved3;
};

// AffinityX2APIC omitted, since the APIC ID is sufficient for our purposes

// Static Resource Affinity Table
struct SRAT
{
	AcpiTable header;
	u32 reserved1;
	u8 reserved2[8];
	AffinityHeader affinities[1];
};

#pragma pack(pop)

template<class Affinity>
static const Affinity* DynamicCastFromHeader(const AffinityHeader* header)
{
	if(header->type != Affinity::type)
		return 0;

	// sanity check: ensure no padding was inserted
	ENSURE(header->length == sizeof(Affinity));

	const Affinity* affinity = (const Affinity*)header;
	if(!IsBitSet(affinity->flags, 0))	// not enabled
		return 0;

	return affinity;
}

struct ProximityDomain
{
	uintptr_t processorMask;
	// (AffinityMemory's fields are not currently needed)
};

typedef std::map<u32, ProximityDomain> ProximityDomains;

static ProximityDomains ExtractProximityDomainsFromSRAT(const SRAT* srat)
{
	ProximityDomains proximityDomains;

	for(const AffinityHeader* header = srat->affinities;
		header < (const AffinityHeader*)(uintptr_t(srat)+srat->header.size);
		header = (const AffinityHeader*)(uintptr_t(header) + header->length))
	{
		const AffinityAPIC* affinityAPIC = DynamicCastFromHeader<AffinityAPIC>(header);
		if(affinityAPIC)
		{
			const size_t processor = ProcessorFromApicId(affinityAPIC->apicId);
			const u32 proximityDomainNumber = affinityAPIC->ProximityDomainNumber();
			ProximityDomain& proximityDomain = proximityDomains[proximityDomainNumber];
			proximityDomain.processorMask |= Bit<uintptr_t>(processor);
		}
	}

	return proximityDomains;
}

static void PopulateNodesFromProximityDomains(const ProximityDomains& proximityDomains)
{
	for(ProximityDomains::const_iterator it = proximityDomains.begin(); it != proximityDomains.end(); ++it)
	{
		const u32 proximityDomainNumber = it->first;
		const ProximityDomain& proximityDomain = it->second;

		Node* node = FindNodeWithProcessorMask(proximityDomain.processorMask);
		if(!node)
			node = AddNode();
		// (we don't know Windows' nodeNumber; it has hopefully already been set)
		node->proximityDomainNumber = proximityDomainNumber;
		node->processorMask = proximityDomain.processorMask;
	}
}

#endif	// #if ARCH_X86_X64


//-----------------------------------------------------------------------------

static ModuleInitState initState;

static Status InitTopology()
{
	PopulateNodes();

#if ARCH_X86_X64
	const SRAT* srat = (const SRAT*)acpi_GetTable("SRAT");
	if(srat && AreApicIdsReliable())
	{
		const ProximityDomains proximityDomains = ExtractProximityDomainsFromSRAT(srat);
		PopulateNodesFromProximityDomains(proximityDomains);
	}
#endif

	// neither OS nor ACPI information is available
	if(numNodes == 0)
	{
		// add dummy node that contains all system processors
		Node* node = AddNode();
		node->nodeNumber = 0;
		node->proximityDomainNumber = 0;
		node->processorMask = os_cpu_ProcessorMask();
	}

	return INFO::OK;
}

size_t numa_NumNodes()
{
	(void)ModuleInit(&initState, InitTopology);
	return numNodes;
}

size_t numa_NodeFromProcessor(size_t processor)
{
	(void)ModuleInit(&initState, InitTopology);
	ENSURE(processor < os_cpu_NumProcessors());
	Node* node = FindNodeWithProcessor(processor);
	ENSURE(node);
	return nodes-node;
}

uintptr_t numa_ProcessorMaskFromNode(size_t node)
{
	(void)ModuleInit(&initState, InitTopology);
	ENSURE(node < numNodes);
	return nodes[node].processorMask;
}

static UCHAR NodeNumberFromNode(size_t node)
{
	(void)ModuleInit(&initState, InitTopology);
	ENSURE(node < numa_NumNodes());
	return nodes[node].nodeNumber;
}


//-----------------------------------------------------------------------------
// memory info

size_t numa_AvailableMemory(size_t node)
{
	// note: it is said that GetNumaAvailableMemoryNode sometimes incorrectly
	// reports zero bytes. the actual cause may however be unexpected
	// RAM configuration, e.g. not all slots filled.
	WUTIL_FUNC(pGetNumaAvailableMemoryNode, BOOL, (UCHAR, PULONGLONG));
	WUTIL_IMPORT_KERNEL32(GetNumaAvailableMemoryNode, pGetNumaAvailableMemoryNode);
	if(pGetNumaAvailableMemoryNode)
	{
		const UCHAR nodeNumber = NodeNumberFromNode(node);
		ULONGLONG availableBytes;
		const BOOL ok = pGetNumaAvailableMemoryNode(nodeNumber, &availableBytes);
		WARN_IF_FALSE(ok);
		const size_t availableMiB = size_t(availableBytes / MiB);
		return availableMiB;
	}
	// NUMA not supported - return available system memory
	else
		return os_cpu_MemoryAvailable();
}


#pragma pack(push, 1)

// ACPI System Locality Information Table
// (System Locality == Proximity Domain)
struct SLIT
{
	AcpiTable header;
	u64 numSystemLocalities;
	u8 entries[1];		// numSystemLocalities*numSystemLocalities entries
};

#pragma pack(pop)

static double ReadRelativeDistanceFromSLIT(const SLIT* slit)
{
	const size_t n = slit->numSystemLocalities;
	ENSURE(slit->header.size == sizeof(SLIT)-sizeof(slit->entries)+n*n);
	// diagonals are specified to be 10
	for(size_t i = 0; i < n; i++)
		ENSURE(slit->entries[i*n+i] == 10);
	// entries = relativeDistance * 10
	return *std::max_element(slit->entries, slit->entries+n*n) / 10.0;
}

// @return ratio between max/min time required to access one node's
// memory from each processor.
static double MeasureRelativeDistance()
{
	const size_t size = 32*MiB;
	void* mem = vm::Allocate(size);
	ASSUME_ALIGNED(mem, pageSize);

	const uintptr_t previousProcessorMask = os_cpu_SetThreadAffinityMask(os_cpu_ProcessorMask());

	double minTime = 1e10, maxTime = 0.0;
	for(size_t node = 0; node < numa_NumNodes(); node++)
	{
		const uintptr_t processorMask = numa_ProcessorMaskFromNode(node);
		os_cpu_SetThreadAffinityMask(processorMask);

		const double startTime = timer_Time();
		memset(mem, 0, size);
		const double elapsedTime = timer_Time() - startTime;

		minTime = std::min(minTime, elapsedTime);
		maxTime = std::max(maxTime, elapsedTime);
	}

	(void)os_cpu_SetThreadAffinityMask(previousProcessorMask);

	vm::Free(mem, size);

	return maxTime / minTime;
}

static double relativeDistance;

static Status InitRelativeDistance()
{
	// early-out for non-NUMA systems (saves some time)
	if(numa_NumNodes() == 1)
	{
		relativeDistance = 1.0;
		return INFO::OK;
	}

	// trust values reported by the BIOS, if available
	const SLIT* slit = (const SLIT*)acpi_GetTable("SLIT");
	if(slit)
		relativeDistance = ReadRelativeDistanceFromSLIT(slit);
	else
		relativeDistance = MeasureRelativeDistance();

	ENSURE(relativeDistance >= 1.0);
	ENSURE(relativeDistance <= 4.0);
	return INFO::OK;
}

double numa_Factor()
{
	static ModuleInitState initState;
	(void)ModuleInit(&initState, InitRelativeDistance);
	return relativeDistance;
}


static bool IsMemoryInterleaved()
{
	if(numa_NumNodes() == 1)
		return false;

	if(!acpi_GetTable("FACP"))	// no ACPI tables available
		return false;	// indeterminate, assume not interleaved

	if(acpi_GetTable("SRAT"))	// present iff not interleaved
		return false;

	return true;
}

static bool isMemoryInterleaved;

static Status InitMemoryInterleaved()
{
	isMemoryInterleaved = IsMemoryInterleaved();
	return INFO::OK;
}

bool numa_IsMemoryInterleaved()
{
	static ModuleInitState initState;
	(void)ModuleInit(&initState, InitMemoryInterleaved);
	return isMemoryInterleaved;
}


//-----------------------------------------------------------------------------

#if 0

static bool VerifyPages(void* mem, size_t size, size_t pageSize, size_t node)
{
	WUTIL_FUNC(pQueryWorkingSetEx, BOOL, (HANDLE, PVOID, DWORD));
	WUTIL_IMPORT_KERNEL32(QueryWorkingSetEx, pQueryWorkingSetEx);
	if(!pQueryWorkingSetEx)
		return true;	// can't do anything

#if WINVER >= 0x600
	size_t largePageSize = os_cpu_LargePageSize();
	ENSURE(largePageSize != 0); // this value is needed for later

	// retrieve attributes of all pages constituting mem
	const size_t numPages = (size + pageSize-1) / pageSize;
	PSAPI_WORKING_SET_EX_INFORMATION* wsi = new PSAPI_WORKING_SET_EX_INFORMATION[numPages];
	for(size_t i = 0; i < numPages; i++)
		wsi[i].VirtualAddress = (u8*)mem + i*pageSize;
	pQueryWorkingSetEx(GetCurrentProcess(), wsi, DWORD(sizeof(PSAPI_WORKING_SET_EX_INFORMATION)*numPages));

	// ensure each is valid and allocated on the correct node
	for(size_t i = 0; i < numPages; i++)
	{
		const PSAPI_WORKING_SET_EX_BLOCK& attributes = wsi[i].VirtualAttributes;
		if(!attributes.Valid)
			return false;
		if((attributes.LargePage != 0) != (pageSize == largePageSize))
		{
			debug_printf("NUMA: is not a large page\n");
			return false;
		}
		if(attributes.Node != node)
		{
			debug_printf("NUMA: allocated from remote node\n");
			return false;
		}
	}

	delete[] wsi;
#else
	UNUSED2(mem);
	UNUSED2(size);
	UNUSED2(pageSize);
	UNUSED2(node);
#endif

	return true;
}

#endif
