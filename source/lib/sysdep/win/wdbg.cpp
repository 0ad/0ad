// Windows debug backend
// Copyright (c) 2002-2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include <stdlib.h>
#include <stdio.h>

#include "lib.h"
#include "win_internal.h"
#include "posix.h"
#include "sysdep/cpu.h"
#include "wdbg.h"
#include "byte_order.h"		// FOURCC

// optional: enables translation of the "unhandled exception" dialog.
#ifdef I18N
#include "ps/i18n.h"
#endif


#pragma data_seg(WIN_CALLBACK_PRE_MAIN(b))
WIN_REGISTER_FUNC(wdbg_init);
#pragma data_seg()



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



// return localized version of <text>, if i18n functionality is available.
// this is used to translate the "unhandled exception" dialog strings.
// WARNING: leaks memory returned by wcsdup, but that's ok since the
// program will terminate soon after. fixing this is hard and senseless.
static const wchar_t* translate(const wchar_t* text)
{
#ifdef HAVE_I18N
	// make sure i18n system is (already|still) initialized.
	if(g_CurrentLocale)
	{
		// be prepared for this to fail, because translation potentially
		// involves script code and the JS context might be corrupted.
		__try
		{
			const wchar_t* text2 = wcsdup(I18n::translate(text).c_str());
			// only overwrite if wcsdup succeeded, i.e. not out of memory.
			if(text2)
				text = text2;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
#endif

	return text;
}


// convenience wrapper using translate.
static void translate_and_display_msg(const wchar_t* caption, const wchar_t* text)
{
	wdisplay_msg(translate(caption), translate(text));
}



//////////////////////////////////////////////////////////////////////////////


// need to shoehorn printf-style variable params into
// the OutputDebugString call.
// - don't want to split into multiple calls - would add newlines to output.
// - fixing Win32 _vsnprintf to return # characters that would be written,
//   as required by C99, looks difficult and unnecessary. if any other code
//   needs that, implement GNU vasprintf.
// - fixed size buffers aren't nice, but much simpler than vasprintf-style
//   allocate+expand_until_it_fits. these calls are for quick debug output,
//   not loads of data, anyway.

// max # characters (including \0) output by debug_(w)printf in one call.
static const int MAX_CNT = 512;

void debug_printf(const char* fmt, ...)
{
	char buf[MAX_CNT];
	buf[MAX_CNT-1] = '\0';

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, MAX_CNT-1, fmt, ap);
	va_end(ap);

	OutputDebugStringA(buf);
}

void debug_wprintf(const wchar_t* fmt, ...)
{
	wchar_t buf[MAX_CNT];
	buf[MAX_CNT-1] = '\0';

	va_list ap;
	va_start(ap, fmt);
	vswprintf(buf, MAX_CNT-1, fmt, ap);
	va_end(ap);

	OutputDebugStringW(buf);
}


//-----------------------------------------------------------------------------
// debug memory allocator
//-----------------------------------------------------------------------------

void debug_heap_check()
{
	__try
	{
		_heapchk();
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}


void debug_heap_enable(DebugHeapChecks what)
{
#ifdef HAVE_VC_DEBUG_ALLOC
	uint flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	switch(what)
	{
	case DEBUG_HEAP_NONE:
		flags = 0;
		break;
	case DEBUG_HEAP_NORMAL:
		flags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
		break;
	case DEBUG_HEAP_ALL:
		flags |= _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF |
		         _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
		break;
	default:
		assert2("debug_heap_enable: invalid what");
	}
	_CrtSetDbgFlag(flags);
#endif // HAVE_DEBUGALLOC
}



//-----------------------------------------------------------------------------


// to avoid deadlock, be VERY CAREFUL to avoid anything that may block,
// including locks taken by the OS (e.g. malloc, GetProcAddress).
typedef int(*WhileSuspendedFunc)(HANDLE hThread, void* user_arg);

struct WhileSuspendedParam
{
	HANDLE hThread;
	WhileSuspendedFunc func;
	void* user_arg;
};


static void* while_suspended_thread_func(void* user_arg)
{
	DWORD err;
	WhileSuspendedParam* param = (WhileSuspendedParam*)user_arg;

	err = SuspendThread(param->hThread);
	// abort, since GetThreadContext only works if the target is suspended.
	if(err == (DWORD)-1)
	{
		debug_warn("while_suspended_thread_func: SuspendThread failed");
		goto fail;
	}
	// target is now guaranteed to be suspended,
	// since the Windows counter never goes negative.

	int ret = param->func(param->hThread, param->user_arg);

	err = ResumeThread(param->hThread);
	assert(err != 0);

	return (void*)(intptr_t)ret;

fail:
	return (void*)(intptr_t)-1;
}


static int call_while_suspended(WhileSuspendedFunc func, void* user_arg)
{
	int err;

	// we need a real HANDLE to the target thread for use with
	// Suspend|ResumeThread and GetThreadContext.
	// alternative: DuplicateHandle on the current thread pseudo-HANDLE.
	// this way is a bit more obvious/simple.
	const DWORD access = THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_SUSPEND_RESUME;
	HANDLE hThread = OpenThread(access, FALSE, GetCurrentThreadId());
	if(hThread == INVALID_HANDLE_VALUE)
	{
		debug_warn("OpenThread failed");
		return -1;
	}

	WhileSuspendedParam param = { hThread, func, user_arg };

	pthread_t thread;
	err = pthread_create(&thread, 0, while_suspended_thread_func, &param);
	assert2(err == 0);

	void* ret;
	err = pthread_join(thread, &ret);
	assert2(err == 0 && ret == 0);

	return (int)(intptr_t)ret;
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

static const uint MAX_BREAKPOINTS = 4;
	// IA-32 limit; if this changes, make sure brk_enable still works!
	// (we assume CONTEXT has contiguous Dr0..Dr3 register fields)


// remove all breakpoints enabled by debug_set_break from <context>.
// called while target is suspended.
static int brk_disable_all_in_ctx(BreakInfo* bi, CONTEXT* context)
{
	context->Dr7 &= ~brk_all_local_enables;
	return 0;
}


// find a free register, set type according to <bi> and
// mark it as enabled in <context>.
// called while target is suspended.
static int brk_enable_in_ctx(BreakInfo* bi, CONTEXT* context)
{
	int reg;	// index (0..3) of first free reg
	uint LE;	// local enable bit for <reg>

	// find free debug register.
	for(reg = 0; reg < MAX_BREAKPOINTS; reg++)
	{
		LE = BIT(reg*2);
		// .. this one is currently not in use.
		if((context->Dr7 & LE) == 0)
			goto have_reg;
	}
	debug_warn("brk_enable_in_ctx: no register available");
	return ERR_LIMIT;
have_reg:

	// set value and mark as enabled.
	(&context->Dr0)[reg] = (DWORD)bi->addr;	// see MAX_BREAKPOINTS
	context->Dr7 |= LE;
	brk_all_local_enables |= LE;

	// build Debug Control Register value.
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
		debug_warn("brk_enable_in_ctx: invalid type");
	}
	// .. length (determined from addr's alignment).
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
	const uint shift = (16 + reg*4);
	const uint field = (len << 2) | rw;

	// clear previous contents of this reg's field
	// (in case the previous user didn't do so on disabling).
	const uint mask = 0xFu << shift;
	context->Dr7 &= ~mask;

	context->Dr7 |= field << shift;
	return 0;
}


// carry out the request stored in the BreakInfo* parameter.
// called while target is suspended.
static int brk_do_request(HANDLE hThread, void* arg)
{
	int ret;
	BreakInfo* bi = (BreakInfo*)arg;

	CONTEXT context;
	context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	if(!GetThreadContext(hThread, &context))
	{
		debug_warn("brk_do_request: GetThreadContext failed");
		goto fail;
	}

#if defined(_M_IX86)
	if(bi->want_all_disabled)
		ret = brk_disable_all_in_ctx(bi, &context);
	else
		ret = brk_enable_in_ctx     (bi, &context);

	if(!SetThreadContext(hThread, &context))
	{
		debug_warn("brk_do_request: SetThreadContext failed");
		goto fail;
	}
#else
#error "port"
#endif

	return 0;
fail:
	return -1;
}


// arrange for a debug exception to be raised when <addr> is accessed
// according to <type>.
// for simplicity, the length (range of bytes to be checked) is
// derived from addr's alignment, and is typically 1 machine word.
// breakpoints are a limited resource (4 on IA-32); abort and
// return ERR_LIMIT if none are available.
int debug_set_break(void* p, DbgBreakType type)
{
	lock();

	brk_info.addr = (uintptr_t)p;
	brk_info.type = type;
	int ret = call_while_suspended(brk_do_request, &brk_info);

	unlock();
	return ret;
}


// remove all breakpoints that were set by debug_set_break.
// important, since these are a limited resource.
int debug_remove_all_breaks()
{
	lock();

	brk_info.want_all_disabled = true;
	int ret = call_while_suspended(brk_do_request, &brk_info);
	brk_info.want_all_disabled = false;

	unlock();
	return ret;
}


//////////////////////////////////////////////////////////////////////////////
//
// exception handler
//
//////////////////////////////////////////////////////////////////////////////

/*

CSmartHandle hImpersonationToken = NULL;
if(!GetImpersonationToken(&hImpersonationToken.m_h))
{
	return FALSE;
}

// We need the SeDebugPrivilege to be able to run MiniDumpWriteDump
TOKEN_PRIVILEGES tp;
BOOL bPrivilegeEnabled = EnablePriv(SE_DEBUG_NAME, hImpersonationToken, &tp);

// DBGHELP.DLL is not thread safe
EnterCriticalSection(pCS);
bRet = pDumpFunction(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithDataSegs, &stInfo, NULL, NULL);
LeaveCriticalSection(pCS);

if(bPrivilegeEnabled)
{
	// Restore the privilege
	RestorePriv(hImpersonationToken, &tp);
}
static BOOL GetImpersonationToken(HANDLE* phToken)
{
	*phToken = NULL;

	if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, TRUE, phToken))
	{
		if(GetLastError() == ERROR_NO_TOKEN)
		{
			// No impersonation token for the curren thread available - go for the process token
			if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, phToken))
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

static BOOL EnablePriv(LPCTSTR pszPriv, HANDLE hToken, TOKEN_PRIVILEGES* ptpOld)
{
	BOOL bOk = FALSE;

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	bOk = LookupPrivilegeValue( 0, pszPriv, &tp.Privileges[0].Luid);
	if(bOk)
	{
		DWORD cbOld = sizeof(*ptpOld);
		bOk = AdjustTokenPrivileges(hToken, FALSE, &tp, cbOld, ptpOld, &cbOld);
	}

	return (bOk && (ERROR_NOT_ALL_ASSIGNED != GetLastError()));
}

static BOOL RestorePriv(HANDLE hToken, TOKEN_PRIVILEGES* ptpOld)
{
	BOOL bOk = AdjustTokenPrivileges(hToken, FALSE, ptpOld, 0, 0, 0);	
	return (bOk && (ERROR_NOT_ALL_ASSIGNED != GetLastError()));
}




























BOOL SetPrivilege(
HANDLE hToken,          // token handle
LPCTSTR Privilege,      // Privilege to enable/disable
BOOL bEnablePrivilege   // TRUE to enable.  FALSE to disable
)
{
TOKEN_PRIVILEGES tp;
LUID luid;
TOKEN_PRIVILEGES tpPrevious;
DWORD cbPrevious=sizeof(TOKEN_PRIVILEGES);

if(!LookupPrivilegeValue( NULL, Privilege, &luid )) return FALSE;

//
// first pass.  get current privilege setting
//
tp.PrivilegeCount           = 1;
tp.Privileges[0].Luid       = luid;
tp.Privileges[0].Attributes = 0;

AdjustTokenPrivileges(
hToken,
FALSE,
&tp,
sizeof(TOKEN_PRIVILEGES),
&tpPrevious,
&cbPrevious
);

if (GetLastError() != ERROR_SUCCESS) return FALSE;

//
// second pass.  set privilege based on previous setting
//
tpPrevious.PrivilegeCount       = 1;
tpPrevious.Privileges[0].Luid   = luid;

if(bEnablePrivilege) {
tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
}
else {
tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
tpPrevious.Privileges[0].Attributes);
}

AdjustTokenPrivileges(
hToken,
FALSE,
&tpPrevious,
cbPrevious,
NULL,
NULL
);

if (GetLastError() != ERROR_SUCCESS) return FALSE;

return TRUE;
}
BOOL SetPrivilege2(
HANDLE hToken,  // token handle
LPCTSTR Privilege,  // Privilege to enable/disable
BOOL bEnablePrivilege  // TRUE to enable. FALSE to disable
)
{
TOKEN_PRIVILEGES tp = { 0 };
// Initialize everything to zero
LUID luid;
DWORD cb=sizeof(TOKEN_PRIVILEGES);
if(!LookupPrivilegeValue( NULL, Privilege, &luid ))
return FALSE;
tp.PrivilegeCount = 1;
tp.Privileges[0].Luid = luid;
if(bEnablePrivilege) {
tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
} else {
tp.Privileges[0].Attributes = 0;
}
AdjustTokenPrivileges( hToken, FALSE, &tp, cb, NULL, NULL );
if (GetLastError() != ERROR_SUCCESS)
return FALSE;

return TRUE;
}

extern WINBASEAPI LANGID WINAPI GetSystemDefaultLangID (void);

void DisplayError(
LPTSTR szAPI    // pointer to failed API name
)
{
LPTSTR MessageBuffer;
DWORD dwBufferLength;

fprintf(stderr,"%s() error!\n", szAPI);
/*
if(dwBufferLength=FormatMessage(
FORMAT_MESSAGE_ALLOCATE_BUFFER |
FORMAT_MESSAGE_FROM_SYSTEM,
NULL,
GetLastError(),
GetSystemDefaultLangID(),
(LPTSTR) &MessageBuffer,
0,
NULL
))
{
DWORD dwBytesWritten;

//
// Output message string on stderr
//
WriteFile(
GetStdHandle(STD_ERROR_HANDLE),
MessageBuffer,
dwBufferLength,
&dwBytesWritten,
NULL
);

//
// free the buffer allocated by the system
//
LocalFree(MessageBuffer);
}
}


static int screwaround()
{
	HANDLE hProcess;
	HANDLE hToken;
	int dwRetVal=RTN_OK; // assume success from main()

	if(!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
	{
		if (GetLastError() == ERROR_NO_TOKEN)
		{
			if (!ImpersonateSelf(SecurityImpersonation))
				return RTN_ERROR;

			if(!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken)){
				DisplayError("OpenThreadToken");
				return RTN_ERROR;
			}
		}
		else
			return RTN_ERROR;
	}

	// enable SeDebugPrivilege
	if(!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE))
	{
		DisplayError("SetPrivilege");

		// close token handle
		CloseHandle(hToken);

		// indicate failure
		return RTN_ERROR;
	}

	// disable SeDebugPrivilege
	SetPrivilege(hToken, SE_DEBUG_NAME, FALSE);
}



















*/



//
// analyze exceptions; determine their type and locus
//

// storage for strings built by get_SEH_exception_description and get_cpp_exception_description.
static wchar_t description[128];

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
#ifdef _MSC_VER
	// note: representation of 'msc' isn't specified, so use FOURCC
	if(er->ExceptionCode != FOURCC(0xe0, 'm','s','c'))
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
		swprintf(description, ARRAY_SIZE(description), L"%hs(\"%hs\")", type_name, what);
		return description;
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
		swprintf(description, ARRAY_SIZE(description), translate(fmt), translate(op), ei[1]);
		return description;
	}

	// rationale: we don't use FormatMessage because it is unclear whether
	// NTDLL's symbol table will always include English-language strings
	// (we don't want crashlogs in foreign gobbledygook).
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
	swprintf(description, ARRAY_SIZE(description), L"Unknown (0x%08X)", code);
	return description;
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


// return an indication of where the exception <er> occurred (lang. neutral).
// it is only valid until the next call, since static storage is used.
static const wchar_t* get_exception_locus(const EXCEPTION_POINTERS* ep)
{
	// HACK: <ep> provides no useful information - ExceptionAddress always
	// points to kernel32!RaiseException. we use debug_dump_stack to determine the
	// real location.

	wchar_t buf[32000];
	const wchar_t* stack_trace = debug_dump_stack(buf, ARRAY_SIZE(buf), 1, ep->ContextRecord);

	const size_t MAX_LOCUS_CHARS = 256;
	static wchar_t locus[MAX_LOCUS_CHARS];
	wcsncpy_s(locus, MAX_LOCUS_CHARS, stack_trace, MAX_LOCUS_CHARS-1);
	wchar_t* end = wcschr(locus, '\r');
	if(end)
		*end = '\0';
	return locus;
}


// called* when an SEH exception was not caught by the app;
// provides detailed debugging information and exits.
// (via win.cpp!entry's __except or as a vectored handler; see below) 
//
// note: keep memory allocs and lock usage to an absolute minimum, because we may
// deadlock the process
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

extern void wdbg_write_minidump(EXCEPTION_POINTERS* ep);

LONG WINAPI wdbg_exception_filter(EXCEPTION_POINTERS* ep)
{
	// note: we risk infinite recursion if someone raises an SEH exception
	// from within this function. therefore, abort immediately if we've
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
	const wchar_t* locus       = get_exception_locus      (ep);

	wchar_t func_name[DBG_SYMBOL_LEN]; char file[DBG_FILE_LEN] = {0}; int line = 0; wchar_t fmt[50];
	swprintf(fmt, ARRAY_SIZE(fmt), L"%%%ds %%%dhs (%%d)", DBG_SYMBOL_LEN, DBG_FILE_LEN);
		// bake in the string limits (future-proof)
	if(swscanf(locus, fmt, func_name, file, &line) != 3)
		debug_warn("error extracting file/line from exception locus");

	wchar_t buf[500];
	const wchar_t* msg_fmt =
		L"Much to our regret we must report the program has encountered an error.\r\n"
		L"\r\n"
		L"Please let us know at http://bugs.wildfiregames.com/ and attach the crashlog.txt and crashlog.dmp files.\r\n"
		L"\r\n"
		L"Details: unhandled exception (%s at %s)\r\n";
	swprintf(buf, ARRAY_SIZE(buf), msg_fmt, description, locus);
	int flags = 0;
	if(ep->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
		flags = DE_NO_CONTINUE;
	ErrorReaction er = display_error(buf, flags, 1, ep->ContextRecord, file, line);
	assert(er > 0);

	wdbg_write_minidump(ep);

	// invoke the Win32 default handler - it calls ExitProcess for
	// most exception types.
	return EXCEPTION_CONTINUE_SEARCH;
}


static int wdbg_init(void)
{
	// add vectored exception handler (if supported by the OS).
	// see rationale above.
#if _WIN32_WINNT >= 0x0500	// this is how winbase.h tests for it
	const HMODULE hKernel32Dll = LoadLibrary("kernel32.dll");
	PVOID (WINAPI *pAddVectoredExceptionHandler)(IN ULONG FirstHandler, IN PVECTORED_EXCEPTION_HANDLER VectoredHandler);
	*(void**)&pAddVectoredExceptionHandler = GetProcAddress(hKernel32Dll, "AddVectoredExceptionHandler"); 
	FreeLibrary(hKernel32Dll);
		// make sure the reference is released so BoundsChecker
		// doesn't complain. it won't actually be unloaded anyway -
		// there is at least one other reference.
	if(pAddVectoredExceptionHandler)
		pAddVectoredExceptionHandler(TRUE, wdbg_exception_filter);
#endif

	return 0;
}
