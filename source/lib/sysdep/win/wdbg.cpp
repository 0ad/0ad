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

#include <stdlib.h>
#include <stdio.h>
#include <typeinfo>

#include "lib/lib.h"
#include "lib/posix/posix_pthread.h"
#include "lib/byte_order.h"		// FOURCC
#include "lib/app_hooks.h"
#include "lib/sysdep/cpu.h"
#include "win_internal.h"
#include "wdbg_sym.h"
#include "winit.h"
#include "wutil.h"


#pragma SECTION_PRE_LIBC(D)
WIN_REGISTER_FUNC(wdbg_init);
#pragma FORCE_INCLUDE(wdbg_init)
#pragma SECTION_RESTORE


// used to prevent the vectored exception handler from taking charge when
// an exception is raised from the main thread (allows __try blocks to
// get control). latched in wdbg_init.
static DWORD main_thread_id;


// protects the breakpoint helper thread.
static void lock()
{
	win_lock(WDBG_CS);
}

static void unlock()
{
	win_unlock(WDBG_CS);
}

// used in a desperate attempt to avoid deadlock in wdbg_exception_handler.
static bool is_locked()
{
	return win_is_locked(WDBG_CS) == 1;
}


void debug_puts(const char* text)
{
	OutputDebugStringA(text);
}


//////////////////////////////////////////////////////////////////////////////


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


//////////////////////////////////////////////////////////////////////////////
//
// breakpoints
//
//////////////////////////////////////////////////////////////////////////////

// breakpoints are set by storing the address of interest in a
// debug register and marking it 'enabled'.
//
// the first problem is, they are only accessible from Ring0;
// we get around this by updating their values via SetThreadContext.
// that in turn requires we suspend the current thread,
// spawn a helper to change the registers, and resume.

// parameter passing to helper thread. currently static storage,
// but the struct simplifies switching to a queue later.
static struct BreakInfo
{
	uintptr_t addr;
	DbgBreakType type;

	// determines what brk_thread_func will do.
	// set/reset by debug_remove_all_breaks.
	bool want_all_disabled;
}
brk_info;

// Local Enable bits of all registers we enabled (used when restoring all).
static DWORD brk_all_local_enables;

// IA32 limit; will need to update brk_enable_in_ctx if this changes.
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


//////////////////////////////////////////////////////////////////////////////
//
// exception handler
//
//////////////////////////////////////////////////////////////////////////////

//
// analyze exceptions; determine their type and locus
//

// storage for strings built by get_SEH_exception_description and get_cpp_exception_description.
static wchar_t description_buf[128];

// VC++ exception handling internals.
// see http://www.codeproject.com/cpp/exceptionhandler.asp
struct XTypeInfo
{
	DWORD _;
	const std::type_info* ti;
	// ..
};

struct XTypeInfoArray
{
	DWORD count; 
	const XTypeInfo* types[1];
};

struct XInfo
{
	DWORD _[3];
	const XTypeInfoArray* array;
};


// does the given SEH exception look like a C++ exception?
// (compiler-specific).
static bool isCppException(const EXCEPTION_RECORD* er)
{
#if MSC_VERSION
	// notes:
	// - value of multibyte character constants (e.g. 'msc') aren't
	//   specified by C++, so use FOURCC instead.
	// - "MS C" compiler is the only interpretation of this magic value that
	//   makes sense, so it is apparently stored in big-endian format.
	if(er->ExceptionCode != FOURCC_BE(0xe0, 'm','s','c'))
		return false;

	// exception info = (magic, &thrown_Cpp_object, &XInfo)
	if(er->NumberParameters != 3)
		return false;

	// MAGIC_NUMBER1 from exsup.inc
	if(er->ExceptionInformation[0] != 0x19930520)
		return false;

	return true;
#else
# error "port"
#endif
}


// if <er> is not a C++ exception, return 0. otherwise, return a description
// of the exception type and cause (in English). uses static storage.
static const wchar_t* get_cpp_exception_description(const EXCEPTION_RECORD* er)
{
	if(!isCppException(er))
		return 0;

	// see above for interpretation
	const ULONG_PTR* const ei = er->ExceptionInformation;

	// note: we can't share a __try below - the failure of
	// one attempt must not abort the others.

	// get std::type_info
	char type_buf[100] = {'\0'};
	const char* type_name = type_buf;
	__try
	{
		const XInfo* xi           = (XInfo*)ei[2];
		const XTypeInfoArray* xta = xi->array;
		const XTypeInfo* xti      = xta->types[0];
		const std::type_info* ti  = xti->ti;

		// strip "class " from start of string (clutter)
		strcpy_s(type_buf, ARRAY_SIZE(type_buf), ti->name());
		if(!strncmp(type_buf, "class ", 6))
			type_name += 6;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}

	// std::exception.what()
	char what[100] = {'\0'};
	__try
	{
		std::exception* e = (std::exception*)ei[1];
		strcpy_s(what, ARRAY_SIZE(what), e->what());
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}


	// we got meaningful data; format and return it.
	if(type_name[0] != '\0' || what[0] != '\0')
	{
		swprintf(description_buf, ARRAY_SIZE(description_buf), L"%hs(\"%hs\")", type_name, what);
		return description_buf;
	}

	// not a C++ exception; we can't say anything about it.
	return 0;
}


// return a description of the exception type (in English).
// uses static storage.
static const wchar_t* get_SEH_exception_description(const EXCEPTION_RECORD* er)
{
	const DWORD code = er->ExceptionCode;
	const ULONG_PTR* ei = er->ExceptionInformation;

	// special case for access violations: display type and address.
	if(code == EXCEPTION_ACCESS_VIOLATION)
	{
		const wchar_t* op = (ei[0])? L"writing" : L"reading";
		const wchar_t* fmt = L"Access violation %s 0x%08X";
		swprintf(description_buf, ARRAY_SIZE(description_buf), ah_translate(fmt), ah_translate(op), ei[1]);
		return description_buf;
	}

	// rationale: we don't use FormatMessage because it is unclear whether
	// NTDLL's symbol table will always include English-language strings
	// (we don't want to receive crashlogs in foreign gobbledygook).
	// it also adds unwanted formatting (e.g. {EXCEPTION} and trailing .).

	switch(code)
	{
//	case EXCEPTION_ACCESS_VIOLATION:         return L"Access violation";
	case EXCEPTION_DATATYPE_MISALIGNMENT:    return L"Datatype misalignment";
	case EXCEPTION_BREAKPOINT:               return L"Breakpoint";
	case EXCEPTION_SINGLE_STEP:              return L"Single step";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return L"Array bounds exceeded";
	case EXCEPTION_FLT_DENORMAL_OPERAND:     return L"FPU denormal operand";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return L"FPU divide by zero";
	case EXCEPTION_FLT_INEXACT_RESULT:       return L"FPU inexact result";
	case EXCEPTION_FLT_INVALID_OPERATION:    return L"FPU invalid operation";
	case EXCEPTION_FLT_OVERFLOW:             return L"FPU overflow";
	case EXCEPTION_FLT_STACK_CHECK:          return L"FPU stack check";
	case EXCEPTION_FLT_UNDERFLOW:            return L"FPU underflow";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:       return L"Integer divide by zero";
	case EXCEPTION_INT_OVERFLOW:             return L"Integer overflow";
	case EXCEPTION_PRIV_INSTRUCTION:         return L"Privileged instruction";
	case EXCEPTION_IN_PAGE_ERROR:            return L"In page error";
	case EXCEPTION_ILLEGAL_INSTRUCTION:      return L"Illegal instruction";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: return L"Noncontinuable exception";
	case EXCEPTION_STACK_OVERFLOW:           return L"Stack overflow";
	case EXCEPTION_INVALID_DISPOSITION:      return L"Invalid disposition";
	case EXCEPTION_GUARD_PAGE:               return L"Guard page";
	case EXCEPTION_INVALID_HANDLE:           return L"Invalid handle";
	}

	// anything else => unknown; display its exception code.
	// we don't punt to get_exception_description because anything
	// we get called for will actually be a SEH exception.
	swprintf(description_buf, ARRAY_SIZE(description_buf), L"Unknown (0x%08X)", code);
	return description_buf;
}


// return a description of the exception <er> (in English).
// it is only valid until the next call, since static storage is used.
static const wchar_t* get_exception_description(const EXCEPTION_POINTERS* ep)
{
	const EXCEPTION_RECORD* const er = ep->ExceptionRecord;

	// note: more specific than SEH, so try it first.
	const wchar_t* d = get_cpp_exception_description(er);
	if(d)
		return d;

	return get_SEH_exception_description(er);
}


// return location at which the exception <er> occurred.
// params: see debug_resolve_symbol.
static void get_exception_locus(const EXCEPTION_POINTERS* ep,
	char* file, int* line, char* func)
{
	// HACK: <ep> provides no useful information - ExceptionAddress always
	// points to kernel32!RaiseException. we use debug_get_nth_caller to
	// determine the real location.

	const uint skip = 1;	// skip RaiseException
	void* func_addr = debug_get_nth_caller(skip, ep->ContextRecord);
	(void)debug_resolve_symbol(func_addr, func, file, line);
}


// called* when an SEH exception was not caught by the app;
// provides detailed debugging information and exits.
// (via win.cpp!entry's __except or vectored_exception_handler; see below) 
//
// note: keep memory allocs and locking to an absolute minimum, because
// they may deadlock the process!
//
// rationale:
// we want to replace the OS "program error" dialog box because
// it is not all too helpful in debugging. to that end, there are
// 5 ways to make sure unhandled SEH exceptions are caught:
// - via WaitForDebugEvent; the app is run from a separate debugger process.
//   this complicates analysis, since the exception is in another
//   address space. also, we are basically implementing a full-featured
//   debugger - overkill.
// - by wrapping all threads in __try (necessary since the handler chain
//   is in TLS). this is very difficult to guarantee.
// - with a vectored exception handler. this works across threads, but
//   is never called when the process is being debugged
//   (messing with the PEB flag doesn't help; root cause is the
//   Win32 KiUserExceptionDispatcher implementation).
//   worse, it's only available on WinXP (unacceptable).
// - by setting the per-process unhandled exception filter. as above,
//   this works across threads and isn't called while a debugger is active;
//   it is at least portable across Win32. unfortunately, some Win32 DLLs
//   appear to register their own handlers, so this isn't reliable.
// - by hooking the exception dispatcher. this isn't future-proof.
//
// so, SNAFU. we compromise and register a regular __except filter at
// program entry point and add a vectored exception handler
// (if supported by the OS) to cover other threads.
// we steer clear of the process-wide unhandled exception filter,
// because it is not understood in what cases it can be overwritten;
// this precludes reliable use.
//
// since C++ exceptions are implemented via SEH, we can also catch those here;
// it's nicer than a global try{} and avoids duplicating this code.
// we can still get at the C++ information (std::exception.what()) by
// examining the internal exception data structures. these are
// compiler-specific, but haven't changed from VC5-VC7.1.
// alternatively, _set_se_translator could be used to translate all
// SEH exceptions to C++. this way is more reliable/documented, but has
// several drawbacks:
// - it wouldn't work at all in C programs,
// - a new fat exception class would have to be created to hold the
//   SEH exception information (e.g. CONTEXT for a stack trace), and
// - this information would not be available for C++ exceptions.
LONG WINAPI wdbg_exception_filter(EXCEPTION_POINTERS* ep)
{
	// note: we risk infinite recursion if someone raises an SEH exception
	// from within this function. therefore, abort immediately if we have
	// already been called; the first error is the most important, anyway.
	static uintptr_t already_crashed = 0;
	if(!CAS(&already_crashed, 0, 1))
		return EXCEPTION_CONTINUE_SEARCH;

	// someone is already holding the dbghelp lock - this is bad.
	// we'll report this problem first and then try to display the
	// exception info regardless (maybe dbghelp won't blow up).
	if(is_locked())
		DISPLAY_ERROR(L"Exception raised while critical section is held - may deadlock..");

	// extract details from ExceptionRecord.
	const wchar_t* description = get_exception_description(ep);
	char file[DBG_FILE_LEN] = {0};
	int line = 0;
	char func_name[DBG_SYMBOL_LEN] = {0};
	get_exception_locus(ep, file, &line, func_name);

	// this must happen before the error dialog because user could choose to
	// exit immediately there.
	wdbg_sym_write_minidump(ep);

	wchar_t buf[500];
	const wchar_t* msg_fmt =
		L"Much to our regret we must report the program has encountered an error.\r\n"
		L"\r\n"
		L"Please let us know at http://bugs.wildfiregames.com/ and attach the crashlog.txt and crashlog.dmp files.\r\n"
		L"\r\n"
		L"Details: unhandled exception (%s)\r\n";
	swprintf(buf, ARRAY_SIZE(buf), msg_fmt, description);
	uint flags = 0;

	if(ep->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
		flags = DE_NO_CONTINUE;
	ErrorReaction er = debug_display_error(buf, flags, 1,ep->ContextRecord, file,line,func_name, NULL);
	debug_assert(er == ER_CONTINUE);	// nothing else possible

	// invoke the Win32 default handler - it calls ExitProcess for
	// most exception types.
	return EXCEPTION_CONTINUE_SEARCH;
}


static LONG WINAPI vectored_exception_handler(EXCEPTION_POINTERS* ep)
{
	// since we're called from the vectored handler chain,
	// ignore exceptions from the main thread. this allows
	// __try blocks to take charge; entry() catches all exceptions with a
	// standard filter and relays them to wdbg_exception_filter.
	if(main_thread_id == GetCurrentThreadId())
		return EXCEPTION_CONTINUE_SEARCH;
	return wdbg_exception_filter(ep);
}


static LibError wdbg_init(void)
{
	// see decl
	main_thread_id = GetCurrentThreadId();

	// add vectored exception handler (if supported by the OS).
	// see rationale above.
#if _WIN32_WINNT >= 0x0500	// this is how winbase.h tests for it
	const HMODULE hKernel32Dll = LoadLibrary("kernel32.dll");
	PVOID (WINAPI *pAddVectoredExceptionHandler)(IN ULONG FirstHandler, IN PVECTORED_EXCEPTION_HANDLER VectoredHandler);
	*(void**)&pAddVectoredExceptionHandler = GetProcAddress(hKernel32Dll, "AddVectoredExceptionHandler"); 
	FreeLibrary(hKernel32Dll);
	if(pAddVectoredExceptionHandler)
		pAddVectoredExceptionHandler(TRUE, vectored_exception_handler);
#endif

	return INFO::OK;
}




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


static NT_TIB* get_tib()
{
	NT_TIB* tib;
	__asm
	{
		mov		eax, fs:[NT_TIB.Self]
		mov		[tib], eax
	}
	return tib;
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



