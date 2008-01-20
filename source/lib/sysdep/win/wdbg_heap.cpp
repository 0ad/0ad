#include "precompiled.h"
#include "wdbg_heap.h"

#include <crtdbg.h>
#include <excpt.h>
#include "winit.h"

WINIT_REGISTER_CRITICAL_INIT(wdbg_heap_Init);

// CAUTION: called from critical init
void wdbg_heap_Enable(bool enable)
{
	uint flags = 0;
	if(enable)
	{
		flags |= _CRTDBG_ALLOC_MEM_DF;	// enable checks at deallocation time
		flags |= _CRTDBG_LEAK_CHECK_DF;	// report leaks at exit
#if CONFIG_PARANOIA
		flags |= _CRTDBG_CHECK_ALWAYS_DF;	// check during every heap operation (slow!)
		flags |= _CRTDBG_DELAY_FREE_MEM_DF;	// blocks cannot be reused
#endif
	}
	_CrtSetDbgFlag(flags);

	// Send output to stdout as well as the debug window, so it works during
	// the normal build process as well as when debugging the test .exe
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
}


void wdbg_heap_Validate()
{
	int ret;
	__try
	{
		// NB: this is a no-op if !_CRTDBG_ALLOC_MEM_DF.
		// we could call _heapchk but that would catch fewer errors.
		ret = _CrtCheckMemory();
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = _HEAPBADNODE;
	}

	if(ret != _HEAPOK)
		DEBUG_DISPLAY_ERROR(L"Heap is corrupted!");
}


//-----------------------------------------------------------------------------

static int __cdecl ReportHook(int reportType, char* message, int* shouldTriggerBreakpoint)
{
	if(message[0] == '{')
	{
		debug_printf("leak\n");
	}

	*shouldTriggerBreakpoint = 0;
	return 1;	// CRT is to display the message as normal
}

static _CRT_ALLOC_HOOK previousAllocHook;

/**
 * @param userData is only valid (nonzero) for allocType == _HOOK_FREE because
 * we are called BEFORE the actual heap operation.
 **/
static int __cdecl AllocHook(int allocType, void* userData, size_t size, int blockType, long requestNumber, const unsigned char* file, int line)
{
	if(previousAllocHook)
		return previousAllocHook(allocType, userData, size, blockType, requestNumber, file, line);
	return 1;	// continue as if the hook had never been called
}



// NB: called from critical init => can't use debug.h services
static void InstallHooks()
{
	int ret = _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, ReportHook);
	if(ret == -1)
		abort();

	previousAllocHook = _CrtSetAllocHook(AllocHook);
}

static LibError wdbg_heap_Init()
{
	InstallHooks();
	wdbg_heap_Enable(true);
	return INFO::OK;
}
