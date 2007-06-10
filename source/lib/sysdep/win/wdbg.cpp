/**
 * =========================================================================
 * File        : wdbg.cpp
 * Project     : 0 A.D.
 * Description : Win32 debug support code and exception handler.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "wdbg.h"

#include "lib/bits.h"
#include "win.h"
#include "wutil.h"


// protects the breakpoint helper thread.
static void lock()
{
	win_lock(WDBG_CS);
}

static void unlock()
{
	win_unlock(WDBG_CS);
}


static NT_TIB* get_tib()
{
#if CPU_IA32
	NT_TIB* tib;
	__asm
	{
		mov		eax, fs:[NT_TIB.Self]
		mov		[tib], eax
	}
	return tib;
#endif
}


//-----------------------------------------------------------------------------
// debug memory allocator
//-----------------------------------------------------------------------------

// check heap integrity (independently of mmgr).
// errors are reported by the CRT or via debug_display_error.
void debug_heap_check()
{
	int ret;
	__try
	{
		ret = _heapchk();
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = _HEAPBADNODE;
	}

	if(ret != _HEAPOK)
		DISPLAY_ERROR(L"debug_heap_check: heap is corrupt");
}


// call at any time; from then on, the specified checks will be performed.
// if not called, the default is DEBUG_HEAP_NONE, i.e. do nothing.
void debug_heap_enable(DebugHeapChecks what)
{
	// note: if mmgr is enabled, crtdbg.h will not have been included.
	// in that case, we do nothing here - this interface is too basic to
	// control mmgr; use its API instead.
#if !CONFIG_USE_MMGR && HAVE_VC_DEBUG_ALLOC
	uint flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	switch(what)
	{
	case DEBUG_HEAP_NONE:
		// note: do not set flags to zero because we might trash some
		// important flag value.
		flags &= ~(_CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_DELAY_FREE_MEM_DF |
		           _CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
		break;
	case DEBUG_HEAP_NORMAL:
		flags |= _CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF;
		break;
	case DEBUG_HEAP_ALL:
		flags |= (_CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_DELAY_FREE_MEM_DF |
		          _CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
		break;
	default:
		debug_assert("debug_heap_enable: invalid what");
	}
	_CrtSetDbgFlag(flags);
#else
	UNUSED2(what);
#endif // HAVE_DEBUGALLOC
}


//-----------------------------------------------------------------------------
// thread suspension

// suspend a thread, execute a user callback, revive the thread.

// to avoid deadlock, be VERY CAREFUL to avoid anything that may block,
// including locks taken by the OS (e.g. malloc, GetProcAddress).
typedef LibError (*WhileSuspendedFunc)(HANDLE hThread, void* user_arg);

struct WhileSuspendedParam
{
	HANDLE hThread;
	WhileSuspendedFunc func;
	void* user_arg;
};


static void* while_suspended_thread_func(void* user_arg)
{
	debug_set_thread_name("suspender");

	WhileSuspendedParam* param = (WhileSuspendedParam*)user_arg;

	DWORD err = SuspendThread(param->hThread);
	// abort, since GetThreadContext only works if the target is suspended.
	if(err == (DWORD)-1)
	{
		debug_warn("while_suspended_thread_func: SuspendThread failed");
		return (void*)(intptr_t)-1;
	}
	// target is now guaranteed to be suspended,
	// since the Windows counter never goes negative.

	LibError ret = param->func(param->hThread, param->user_arg);

	WARN_IF_FALSE(ResumeThread(param->hThread));

	return (void*)(intptr_t)ret;
}


static LibError call_while_suspended(WhileSuspendedFunc func, void* user_arg)
{
	int err;

	// we need a real HANDLE to the target thread for use with
	// Suspend|ResumeThread and GetThreadContext.
	// alternative: DuplicateHandle on the current thread pseudo-HANDLE.
	// this way is a bit more obvious/simple.
	const DWORD access = THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_SUSPEND_RESUME;
	HANDLE hThread = OpenThread(access, FALSE, GetCurrentThreadId());
	if(hThread == INVALID_HANDLE_VALUE)
		WARN_RETURN(ERR::FAIL);

	WhileSuspendedParam param = { hThread, func, user_arg };

	pthread_t thread;
	WARN_ERR(pthread_create(&thread, 0, while_suspended_thread_func, &param));

	void* ret;
	err = pthread_join(thread, &ret);
	debug_assert(err == 0 && ret == 0);

	return (LibError)(intptr_t)ret;
}


//-----------------------------------------------------------------------------
// breakpoints
//-----------------------------------------------------------------------------

// breakpoints are set by storing the address of interest in a
// debug register and marking it 'enabled'.
//
// the first problem is, they are only accessible from Ring0;
// we get around this by updating their values via SetThreadContext.
// that in turn requires we suspend the current thread,
// spawn a helper to change the registers, and resume.

// parameter passing to helper thread. currently static storage,
// but the struct simplifies switching to a queue later.
struct BreakInfo
{
	uintptr_t addr;
	DbgBreakType type;

	// determines what brk_thread_func will do.
	// set/reset by debug_remove_all_breaks.
	bool want_all_disabled;
};

static BreakInfo brk_info;

// Local Enable bits of all registers we enabled (used when restoring all).
static DWORD brk_all_local_enables;

// IA-32 limit; will need to update brk_enable_in_ctx if this changes.
static const uint MAX_BREAKPOINTS = 4;


// remove all breakpoints enabled by debug_set_break from <context>.
// called while target is suspended.
static LibError brk_disable_all_in_ctx(BreakInfo* UNUSED(bi), CONTEXT* context)
{
	context->Dr7 &= ~brk_all_local_enables;
	return INFO::OK;
}


// find a free register, set type according to <bi> and
// mark it as enabled in <context>.
// called while target is suspended.
static LibError brk_enable_in_ctx(BreakInfo* bi, CONTEXT* context)
{
	uint reg;	// index (0..3) of first free reg
	uint LE;	// local enable bit for <reg>

	// find free debug register.
	for(reg = 0; reg < MAX_BREAKPOINTS; reg++)
	{
		LE = BIT(reg*2);
		// .. this one is currently not in use.
		if((context->Dr7 & LE) == 0)
			goto have_reg;
	}
	WARN_RETURN(ERR::LIMIT);
have_reg:

	// store breakpoint address in debug register.
	DWORD addr = (DWORD)bi->addr;
	// .. note: treating Dr0..Dr3 as an array is unsafe due to
	//    possible struct member padding.
	switch(reg)
	{
	case 0:	context->Dr0 = addr; break;
	case 1:	context->Dr1 = addr; break;
	case 2:	context->Dr2 = addr; break;
	case 3:	context->Dr3 = addr; break;
	NODEFAULT;
	}

	// choose breakpoint settings:
	// .. type
	uint rw = 0;
	switch(bi->type)
	{
	case DBG_BREAK_CODE:
		rw = 0; break;
	case DBG_BREAK_DATA:
		rw = 1; break;
	case DBG_BREAK_DATA_WRITE:
		rw = 3; break;
	default:
		debug_warn("invalid type");
	}
	// .. length (determine from addr's alignment).
	//    note: IA-32 requires len=0 for code breakpoints.
	uint len = 0;
	if(bi->type != DBG_BREAK_CODE)
	{
		const uint alignment = (uint)(bi->addr % 4);
		// assume 2 byte range
		if(alignment == 2)
			len = 1;
		// assume 4 byte range
		else if(alignment == 0)
			len = 3;
		// else: 1 byte range; len already set to 0
	}

	// update Debug Control register
	const uint shift = (16 + reg*4);
	// .. clear previous contents of this reg's field
	//    (in case the previous user didn't do so on disabling).
	context->Dr7 &= ~(0xFu << shift);
	// .. write settings
	context->Dr7 |= ((len << 2)|rw) << shift;
	// .. mark as enabled
	context->Dr7 |= LE;

	brk_all_local_enables |= LE;
	return INFO::OK;
}


// carry out the request stored in the BreakInfo* parameter.
// called while target is suspended.
static LibError brk_do_request(HANDLE hThread, void* arg)
{
	LibError ret;
	BreakInfo* bi = (BreakInfo*)arg;

	CONTEXT context;
	context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	if(!GetThreadContext(hThread, &context))
		WARN_RETURN(ERR::FAIL);

#if CPU_IA32
	if(bi->want_all_disabled)
		ret = brk_disable_all_in_ctx(bi, &context);
	else
		ret = brk_enable_in_ctx     (bi, &context);
#else
#error "port"
#endif

	if(!SetThreadContext(hThread, &context))
		WARN_RETURN(ERR::FAIL);

	RETURN_ERR(ret);
	return INFO::OK;
}


// arrange for a debug exception to be raised when <addr> is accessed
// according to <type>.
// for simplicity, the length (range of bytes to be checked) is
// derived from addr's alignment, and is typically 1 machine word.
// breakpoints are a limited resource (4 on IA-32); abort and
// return ERR::LIMIT if none are available.
LibError debug_set_break(void* p, DbgBreakType type)
{
	lock();

	brk_info.addr = (uintptr_t)p;
	brk_info.type = type;
	LibError ret = call_while_suspended(brk_do_request, &brk_info);

	unlock();
	return ret;
}


// remove all breakpoints that were set by debug_set_break.
// important, since these are a limited resource.
LibError debug_remove_all_breaks()
{
	lock();

	brk_info.want_all_disabled = true;
	LibError ret = call_while_suspended(brk_do_request, &brk_info);
	brk_info.want_all_disabled = false;

	unlock();
	return ret;
}


//-----------------------------------------------------------------------------

// return 1 if the pointer appears to be totally bogus, otherwise 0.
// this check is not authoritative (the pointer may be "valid" but incorrect)
// but can be used to filter out obviously wrong values in a portable manner.
int debug_is_pointer_bogus(const void* p)
{
#if CPU_IA32
	if(p < (void*)0x10000)
		return true;
	if(p >= (void*)(uintptr_t)0x80000000)
		return true;
#endif

	// notes:
	// - we don't check alignment because nothing can be assumed about a
	//   string pointer and we mustn't reject any actually valid pointers.
	// - nor do we bother checking the address against known stack/heap areas
	//   because that doesn't cover everything (e.g. DLLs, VirtualAlloc).
	// - cannot use IsBadReadPtr because it accesses the mem
	//   (false alarm for reserved address space).

	return false;
}


bool debug_is_code_ptr(void* p)
{
	uintptr_t addr = (uintptr_t)p;
	// totally invalid pointer
	if(debug_is_pointer_bogus(p))
		return false;
	// comes before load address
	static const HMODULE base = GetModuleHandle(0);
	if(addr < (uintptr_t)base)
		return false;

	return true;
}


bool debug_is_stack_ptr(void* p)
{
	uintptr_t addr = (uintptr_t)p;
	// totally invalid pointer
	if(debug_is_pointer_bogus(p))
		return false;
	// not aligned
	if(addr % sizeof(void*))
		return false;
	// out of bounds (note: IA-32 stack grows downwards)
	NT_TIB* tib = get_tib();
	if(!(tib->StackLimit < p && p < tib->StackBase))
		return false;

	return true;
}


void debug_puts(const char* text)
{
	OutputDebugStringA(text);
}


// inform the debugger of the current thread's description, which it then
// displays instead of just the thread handle.
void wdbg_set_thread_name(const char* name)
{
	// we pass information to the debugger via a special exception it
	// swallows. if not running under one, bail now to avoid
	// "first chance exception" warnings.
	if(!IsDebuggerPresent())
		return;

	// presented by Jay Bazuzi (from the VC debugger team) at TechEd 1999.
	const struct ThreadNameInfo
	{
		DWORD type;
		const char* name;
		DWORD thread_id;	// any valid ID or -1 for current thread
		DWORD flags;
	}
	info = { 0x1000, name, (DWORD)-1, 0 };
	__try
	{
		RaiseException(0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// if we get here, the debugger didn't handle the exception.
		debug_warn("thread name hack doesn't work under this debugger");
	}
}
