#include "precompiled.h"
#include "lib/sysdep/numa.h"

#include "lib/bits.h"	// round_up, PopulationCount
#include "lib/timer.h"
#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/acpi.h"
#include "win.h"
#include "wutil.h"
#include "wcpu.h"
#include "winit.h"
#include <Psapi.h>


WINIT_REGISTER_EARLY_INIT(wnuma_Init);


//-----------------------------------------------------------------------------
// node topology
//-----------------------------------------------------------------------------

static size_t NumNodes()
{
	typedef BOOL (WINAPI *PGetNumaHighestNodeNumber)(PULONG highestNode);
	const HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
	const PGetNumaHighestNodeNumber pGetNumaHighestNodeNumber = (PGetNumaHighestNodeNumber)GetProcAddress(hKernel32, "GetNumaHighestNodeNumber");
	if(pGetNumaHighestNodeNumber)
	{
		ULONG highestNode;
		const BOOL ok = pGetNumaHighestNodeNumber(&highestNode);
		debug_assert(ok);
		debug_assert(highestNode < os_cpu_NumProcessors());	// #nodes <= #processors
		return highestNode+1;
	}
	// NUMA not supported
	else
		return 1;
}


static void FillNodesProcessorMask(uintptr_t* nodesProcessorMask)
{
	typedef BOOL (WINAPI *PGetNumaNodeProcessorMask)(UCHAR node, PULONGLONG affinity);
	const HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
	const PGetNumaNodeProcessorMask pGetNumaNodeProcessorMask = (PGetNumaNodeProcessorMask)GetProcAddress(hKernel32, "GetNumaNodeProcessorMask");
	if(pGetNumaNodeProcessorMask)
	{
		DWORD_PTR processAffinity, systemAffinity;
		const BOOL ok = GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity);
		debug_assert(ok);

		for(size_t node = 0; node < numa_NumNodes(); node++)
		{
			ULONGLONG affinity;
			const BOOL ok = pGetNumaNodeProcessorMask((UCHAR)node, &affinity);
			debug_assert(ok);
			const uintptr_t processorMask = wcpu_ProcessorMaskFromAffinity(processAffinity, (DWORD_PTR)affinity);
			nodesProcessorMask[node] = processorMask;
		}
	}
	// NUMA not supported - consider node 0 to consist of all system processors
	else
		nodesProcessorMask[0] = os_cpu_ProcessorMask();
}


// note: it is easier to implement this in terms of nodesProcessorMask
// rather than the other way around because wcpu provides the
// wcpu_ProcessorMaskFromAffinity helper. there is no similar function to
// convert processor to processorNumber.
static void FillProcessorsNode(size_t numNodes, const uintptr_t* nodesProcessorMask, size_t* processorsNode)
{
	for(size_t node = 0; node < numNodes; node++)
	{
		const uintptr_t processorMask = nodesProcessorMask[node];
		for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
		{
			if(IsBitSet(processorMask, processor))
				processorsNode[processor] = node;
		}
	}
}


//-----------------------------------------------------------------------------
// node topology interface

struct NodeTopology	// POD
{
	size_t numNodes;
	size_t processorsNode[os_cpu_MaxProcessors];
	uintptr_t nodesProcessorMask[os_cpu_MaxProcessors];
};
static NodeTopology s_nodeTopology;

static void DetectNodeTopology()
{
	s_nodeTopology.numNodes = NumNodes();
	FillNodesProcessorMask(s_nodeTopology.nodesProcessorMask);
	FillProcessorsNode(s_nodeTopology.numNodes, s_nodeTopology.nodesProcessorMask, s_nodeTopology.processorsNode);
}

size_t numa_NumNodes()
{
	return s_nodeTopology.numNodes;
}

size_t numa_NodeFromProcessor(size_t processor)
{
	debug_assert(processor < os_cpu_NumProcessors());
	return s_nodeTopology.processorsNode[processor];
}

uintptr_t numa_ProcessorMaskFromNode(size_t node)
{
	debug_assert(node < s_nodeTopology.numNodes);
	return s_nodeTopology.nodesProcessorMask[node];
}


//-----------------------------------------------------------------------------
// memory info
//-----------------------------------------------------------------------------

size_t numa_AvailableMemory(size_t node)
{
	debug_assert(node < numa_NumNodes());

	// note: it is said that GetNumaAvailableMemoryNode sometimes incorrectly
	// reports zero bytes. the actual cause may however be unexpected
	// RAM configuration, e.g. not all slots filled.
	typedef BOOL (WINAPI *PGetNumaAvailableMemoryNode)(UCHAR node, PULONGLONG availableBytes);
	static PGetNumaAvailableMemoryNode pGetNumaAvailableMemoryNode;
	if(!pGetNumaAvailableMemoryNode)
	{
		const HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
		pGetNumaAvailableMemoryNode = (PGetNumaAvailableMemoryNode)GetProcAddress(hKernel32, "GetNumaAvailableMemoryNode");
	}

	if(pGetNumaAvailableMemoryNode)
	{
		ULONGLONG availableBytes;
		const BOOL ok = pGetNumaAvailableMemoryNode((UCHAR)node, &availableBytes);
		debug_assert(ok);
		const size_t availableMiB = size_t(availableBytes / MiB);
		return availableMiB;
	}
	// NUMA not supported - return available system memory
	else
		return os_cpu_MemoryAvailable();
}


double numa_Factor()
{
	WinScopedLock lock(WNUMA_CS);
	static double factor;
	if(factor == 0.0)
	{
		// if non-NUMA, skip the (expensive) measurements below.
		if(numa_NumNodes() == 1)
			factor = 1.0;
		else
		{
			// allocate memory on one node
			const size_t size = 16*MiB;
			shared_ptr<u8> buffer((u8*)numa_AllocateOnNode(size, 0), numa_Deleter<u8>());

			const uintptr_t previousProcessorMask = os_cpu_SetThreadAffinityMask(os_cpu_ProcessorMask());

			// measure min/max fill times required by a processor from each node
			double minTime = 1e10, maxTime = 0.0;
			for(size_t node = 0; node < numa_NumNodes(); node++)
			{
				const uintptr_t processorMask = numa_ProcessorMaskFromNode(node);
				os_cpu_SetThreadAffinityMask(processorMask);

				const double startTime = timer_Time();
				memset(buffer.get(), 0, size);
				const double elapsedTime = timer_Time() - startTime;

				minTime = std::min(minTime, elapsedTime);
				maxTime = std::max(maxTime, elapsedTime);
			}

			(void)os_cpu_SetThreadAffinityMask(previousProcessorMask);

			factor = maxTime / minTime;
		}

		debug_assert(factor >= 1.0);
		debug_assert(factor <= 3.0);	// (Microsoft guideline for NUMA systems)
	}

	return factor;
}


bool numa_IsMemoryInterleaved()
{
	WinScopedLock lock(WNUMA_CS);
	static int isInterleaved = -1;
	if(isInterleaved == -1)
	{
		if(acpi_Init())
		{
			// the BIOS only generates an SRAT (System Resource Affinity Table)
			// if node interleaving is disabled.
			isInterleaved = acpi_GetTable("SRAT") == 0;
			acpi_Shutdown();
		}
		else
			isInterleaved = 0;	// can't tell
	}

	return isInterleaved != 0;
}


//-----------------------------------------------------------------------------
// allocator
//-----------------------------------------------------------------------------

static bool largePageAllocationTookTooLong = false;

static bool ShouldUseLargePages(LargePageDisposition disposition, size_t allocationSize)
{
	// can't, OS does not support large pages
	if(os_cpu_LargePageSize() == 0)
		return false;

	// overrides
	if(disposition == LPD_NEVER)
		return false;
	if(disposition == LPD_ALWAYS)
		return true;

	// default disposition: use a heuristic
	{
		// a previous attempt already took too long (Windows is apparently
		// shoveling aside lots of memory).
		if(largePageAllocationTookTooLong)
			return false;

		// allocation is rather small and would "only" use half of the
		// TLBs for its pages.
		if(allocationSize < 64/2 * os_cpu_PageSize())
			return false;

		// we want there to be plenty of memory available, otherwise the
		// page frames are going to be terribly fragmented and even a
		// single allocation would take SECONDS.
		if(os_cpu_MemoryAvailable() < 2000)	// 2 GB
			return false;
	}

	return true;
}


void* numa_Allocate(size_t size, LargePageDisposition largePageDisposition, size_t* ppageSize)
{
	void* mem = 0;

	// try allocating with large pages (reduces TLB misses)
	if(ShouldUseLargePages(largePageDisposition, size))
	{
		const size_t largePageSize = os_cpu_LargePageSize();
		const size_t paddedSize = round_up(size, largePageSize);	// required by MEM_LARGE_PAGES
		// note: this call can take SECONDS, which is why several checks are
		// undertaken before we even try. these aren't authoritative, so we
		// at least prevent future attempts if it takes too long.
		const double startTime = timer_Time();
		mem = VirtualAlloc(0, paddedSize, MEM_RESERVE|MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
		if(ppageSize)
			*ppageSize = largePageSize;
		const double elapsedTime = timer_Time() - startTime;
		debug_printf("TIMER| NUMA large page allocation: %g\n", elapsedTime);
		if(elapsedTime > 1.0)
			largePageAllocationTookTooLong = true;
	}

	// try (again) with regular pages
	if(!mem)
	{
		mem = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		if(ppageSize)
			*ppageSize = os_cpu_PageSize();
	}

	// all attempts failed - we're apparently out of memory.
	if(!mem)
		throw std::bad_alloc();

	return mem;
}


static bool VerifyPages(void* mem, size_t size, size_t pageSize, size_t node)
{
	typedef BOOL (WINAPI *PQueryWorkingSetEx)(HANDLE hProcess, PVOID buffer, DWORD bufferSize);
	static PQueryWorkingSetEx pQueryWorkingSetEx;
	if(!pQueryWorkingSetEx)
	{
		const HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
		pQueryWorkingSetEx = (PQueryWorkingSetEx)GetProcAddress(hKernel32, "QueryWorkingSetEx");
		if(!pQueryWorkingSetEx)
			return true;	// can't do anything
	}

#if WINVER >= 0x600
	// retrieve attributes of all pages constituting mem
	const size_t numPages = (size + pageSize-1) / pageSize;
	PSAPI_WORKING_SET_EX_INFORMATION* wsi = new PSAPI_WORKING_SET_EX_INFORMATION[numPages];
	for(size_t i = 0; i < numPages; i++)
		wsi[i].VirtualAddress = (u8*)mem + i*pageSize;
	pQueryWorkingSetEx(GetCurrentProcess(), wsi, sizeof(PSAPI_WORKING_SET_EX_INFORMATION)*numPages);

	// ensure each is valid and allocated on the correct node
	for(size_t i = 0; i < numPages; i++)
	{
		const PSAPI_WORKING_SET_EX_BLOCK& attributes = wsi[i].VirtualAttributes;
		if(!attributes.valid)
			return false;
		if(attributes.LargePage != (pageSize == LargePageSize()))
		{
			debug_printf("NUMA: is not a large page\n");
			return false;
		}
		if(attributes.node != node)
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


void* numa_AllocateOnNode(size_t node, size_t size, LargePageDisposition largePageDisposition, size_t* ppageSize)
{
	debug_assert(node < numa_NumNodes());

	// see if there will be enough memory (non-authoritative, for debug purposes only)
	{
		const size_t sizeMiB = size/MiB;
		const size_t availableMiB = numa_AvailableMemory(node);
		if(availableMiB < sizeMiB)
			debug_printf("NUMA: warning: node reports insufficient memory (%d vs %d MB)\n", availableMiB, sizeMiB);
	}

	size_t pageSize;	// (used below even if ppageSize is zero)
	void* const mem = numa_Allocate(size, largePageDisposition, &pageSize);
	if(ppageSize)
		*ppageSize = pageSize;

	// we can't use VirtualAllocExNuma - it's only available in Vista and Server 2008.
	// workaround: fault in all pages now to ensure they are allocated from the
	// current node, then verify page attributes.
	// (note: VirtualAlloc's MEM_COMMIT only maps virtual pages and does not
	// actually allocate page frames. Windows XP uses a first-touch heuristic -
	// the page will be taken from the node whose processor caused the fault.
	// Windows Vista allocates on the "preferred" node, so affinity should be
	// set such that this thread is running on <node>.)
	memset(mem, 0, size);

	VerifyPages(mem, size, pageSize, node);

	return mem;
}


void numa_Deallocate(void* mem)
{
	VirtualFree(mem, 0, MEM_RELEASE);
}


//-----------------------------------------------------------------------------

static LibError wnuma_Init()
{
	DetectNodeTopology();
	return INFO::OK;
}
