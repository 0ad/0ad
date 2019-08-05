/* Copyright (C) 2019 Wildfire Games.
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
 * Structured Exception Handling support
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/wseh.h"

#include "lib/byte_order.h"		// FOURCC
#include "lib/utf8.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/wutil.h"
#include "lib/sysdep/os/win/wdbg_sym.h"			// wdbg_sym_WriteMinidump

#include <process.h>			// __security_init_cookie
#define NEED_COOKIE_INIT

// note: several excellent references are pointed to by comments below.


//-----------------------------------------------------------------------------
// analyze an exception (determine description and locus)

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
static bool IsCppException(const EXCEPTION_RECORD* er)
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

/**
 * @param er An exception record for which IsCppException returned true.
 * @param description
 * @param maxChars
 **/
static const wchar_t* GetCppExceptionDescription(const EXCEPTION_RECORD* er,
	wchar_t* description, size_t maxChars)
{
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
	char what[160] = {'\0'};
	__try
	{
		std::exception* e = (std::exception*)ei[1];
		strcpy_s(what, ARRAY_SIZE(what), e->what());
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}

	// format the info we got (if both are empty, then something is seriously
	// wrong; it's better to show empty strings than returning 0 to have our
	// caller display the SEH info)
	swprintf_s(description, maxChars, L"%hs(\"%hs\")", type_name, what);
	return description;
}


static const wchar_t* GetSehExceptionDescription(const EXCEPTION_RECORD* er,
	wchar_t* description, size_t maxChars)
{
	const DWORD code = er->ExceptionCode;
	const ULONG_PTR* ei = er->ExceptionInformation;

	// rationale: we don't use FormatMessage because it is unclear whether
	// NTDLL's symbol table will always include English-language strings
	// (we don't want to receive crashlogs in foreign gobbledygook).
	// it also adds unwanted formatting (e.g. {EXCEPTION} and trailing .).

	switch(code)
	{
	case EXCEPTION_ACCESS_VIOLATION:
	{
		// special case: display type and address.
		const wchar_t* accessType = (ei[0])? L"writing" : L"reading";
		const ULONG_PTR address = ei[1];
		swprintf_s(description, maxChars, L"Access violation %ls 0x%08X", accessType, address);
		return description;
	}
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
	// we don't punt to GetExceptionDescription because anything
	// we get called for will actually be a SEH exception.
	swprintf_s(description, maxChars, L"Unknown (0x%08X)", code);
	return description;
}


/**
 * @return a description of the exception type and cause (in English).
 **/
static const wchar_t* GetExceptionDescription(const EXCEPTION_POINTERS* ep,
	wchar_t* description, size_t maxChars)
{
	const EXCEPTION_RECORD* const er = ep->ExceptionRecord;

	if(IsCppException(er))
		return GetCppExceptionDescription(er, description, maxChars);
	else
		return GetSehExceptionDescription(er, description, maxChars);
}


// return location at which the exception <er> occurred.
// params: see debug_ResolveSymbol.
static void GetExceptionLocus(EXCEPTION_POINTERS* ep,
	wchar_t* file, int* line, wchar_t* func)
{
	// HACK: <ep> provides no useful information - ExceptionAddress always
	// points to kernel32!RaiseException. we use debug_GetCaller to
	// determine the real location.

	const wchar_t* const lastFuncToSkip = L"RaiseException";
	void* func_addr = debug_GetCaller(ep->ContextRecord, lastFuncToSkip);
	(void)debug_ResolveSymbol(func_addr, func, file, line);
}


//-----------------------------------------------------------------------------
// exception filter

// called when an exception is detected (see below); provides detailed
// debugging information and exits.
//
// note: keep memory allocs and locking to an absolute minimum, because
// they may deadlock the process!
long __stdcall wseh_ExceptionFilter(struct _EXCEPTION_POINTERS* ep)
{
	// OutputDebugString raises an exception, which OUGHT to be swallowed
	// by WaitForDebugEvent but sometimes isn't. if we see it, ignore it.
	if(ep->ExceptionRecord->ExceptionCode == 0x40010006)	// DBG_PRINTEXCEPTION_C
		return EXCEPTION_CONTINUE_EXECUTION;

	// if run in a debugger, let it handle exceptions (tends to be more
	// convenient since it can bring up the crash location)
	if(IsDebuggerPresent())
		return EXCEPTION_CONTINUE_SEARCH;

	// make sure we don't recurse infinitely if this function raises an
	// SEH exception. (we may only have the guard page's 4 KB worth of
	// stack space if the exception is EXCEPTION_STACK_OVERFLOW)
	static intptr_t nestingLevel = 0;
	cpu_AtomicAdd(&nestingLevel, 1);
	if(nestingLevel >= 3)
		return EXCEPTION_CONTINUE_SEARCH;

	// someone is already holding the dbghelp lock - this is bad.
	// we'll report this problem first and then try to display the
	// exception info regardless (maybe dbghelp won't blow up).
	if(wutil_IsLocked(WDBG_SYM_CS) == 1)
		DEBUG_DISPLAY_ERROR(L"Exception raised while critical section is held - may deadlock..");

	// a dump file is essential for debugging, so write it before
	// anything else goes wrong (especially before showing the error
	// dialog because the user could choose to exit immediately)
	wdbg_sym_WriteMinidump(ep);

	// extract details from ExceptionRecord.
	wchar_t descriptionBuf[150];
	const wchar_t* description = GetExceptionDescription(ep, descriptionBuf, ARRAY_SIZE(descriptionBuf));
	wchar_t file[DEBUG_FILE_CHARS] = {0};
	int line = 0;
	wchar_t func[DEBUG_SYMBOL_CHARS] = {0};
	GetExceptionLocus(ep, file, &line, func);

	wchar_t message[600];
	const wchar_t* messageFormat =
		L"Much to our regret we must report the program has encountered an error.\r\n"
		L"\r\n"
		L"Please let us know at http://trac.wildfiregames.com/ and attach the crashlog.txt and crashlog.dmp files.\r\n"
		L"You may find paths to these files at https://trac.wildfiregames.com/wiki/GameDataPaths \r\n"
		L"\r\n"
		L"Details: unhandled exception (%ls)\r\n";
	swprintf_s(message, ARRAY_SIZE(message), messageFormat, description);

	size_t flags = 0;
	if(ep->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
		flags = DE_NO_CONTINUE;
	const wchar_t* const lastFuncToSkip = WIDEN(STRINGIZE(DECORATED_NAME(wseh_ExceptionFilter)));
	ErrorReaction er = debug_DisplayError(message, flags, ep->ContextRecord, lastFuncToSkip, file,line,utf8_from_wstring(func).c_str(), 0);
	ENSURE(er == ER_CONTINUE);	// nothing else possible

	// invoke the Win32 default handler - it calls ExitProcess for
	// most exception types.
	return EXCEPTION_CONTINUE_SEARCH;
}


//-----------------------------------------------------------------------------
// install SEH exception handler

/*

rationale:
we want to replace the OS "program error" dialog box because it is not
all too helpful in debugging. to that end, there are 5 ways to make sure
unhandled SEH exceptions are caught:
- via WaitForDebugEvent; the app is run from a separate debugger process.
  the exception is in another address space, so we'd have to deal with that
  and basically implement a full-featured debugger - overkill.
- by wrapping all threads (their handler chains are in TLS) in __try.
  this can be done with the cooperation of wpthread, but threads not under
  our control aren't covered.
- with a vectored exception handler. this works across threads, but it's
  only available on WinXP (unacceptable). since it is called before __try
  blocks, we would receive expected/legitimate exceptions.
  (see http://msdn.microsoft.com/msdnmag/issues/01/09/hood/default.aspx)
- by setting the per-process unhandled exception filter. as above, this works
  across threads and is at least portable across Win32. unfortunately, some
  Win32 DLLs appear to register their own handlers, so this isn't reliable.
- by hooking the exception dispatcher. this isn't future-proof.

note that the vectored and unhandled-exception filters aren't called when
the process is being debugged (messing with the PEB flag doesn't help;
root cause is the Win32 KiUserExceptionDispatcher implementation).
however, this is fine since the IDE's debugger is more helpful than our
dialog (it is able to jump directly to the offending code).

wrapping all threads in a __try appears to be the best choice. unfortunately,
we cannot retroactively install an SEH handler: the OS ensures SEH chain
nodes are on the thread's stack (as defined by NT_TIB) in ascending order.
(see http://www.microsoft.com/msj/0197/exception/exception.aspx)
the handler would also have to be marked via .safeseh, but that is doable
(see http://blogs.msdn.com/greggm/archive/2004/07/22/191544.aspx)
consequently, we'll have to run within a __try; if the init code is to be
covered, this must happen within the program entry point.

note: since C++ exceptions are implemented via SEH, we can also catch
those here; it's nicer than a global try{} and avoids duplicating this code.
we can still get at the C++ information (std::exception.what()) by examining
the internal exception data structures. these are compiler-specific, but
haven't changed from VC5-VC7.1.
alternatively, _set_se_translator could to translate all SEH exceptions to
C++ classes. this way is more reliable/documented, but has several drawbacks:
- it wouldn't work at all in C programs,
- a new fat exception class would have to be created to hold the
  SEH exception information (e.g. CONTEXT for a stack trace), and
- this information would not be available for C++ exceptions.
(see http://blogs.msdn.com/cbrumme/archive/2003/10/01/51524.aspx)

*/

#ifdef LIB_STATIC_LINK

#include "lib/utf8.h"

EXTERN_C int wmainCRTStartup();

static int CallStartupWithinTryBlock()
{
	int ret;
	__try
	{
		ret = wmainCRTStartup();
	}
	__except(wseh_ExceptionFilter(GetExceptionInformation()))
	{
		ret = -1;
	}
	return ret;
}

EXTERN_C int wseh_EntryPoint()
{
#ifdef NEED_COOKIE_INIT
	// 2006-02-16 workaround for R6035 on VC8:
	//
	// SEH code compiled with /GS pushes a "security cookie" onto the
	// stack. since we're called before CRT init, the cookie won't have
	// been initialized yet, which would cause the CRT to FatalAppExit.
	// to solve this, we must call __security_init_cookie before any
	// hidden compiler-generated SEH registration code runs,
	// which means the __try block must be moved into a helper function.
	//
	// NB: wseh_EntryPoint() must not contain local string buffers,
	// either - /GS would install a cookie here as well (same problem).
	//
	// see http://msdn2.microsoft.com/en-US/library/ms235603.aspx
	__security_init_cookie();
#endif
	return CallStartupWithinTryBlock();
}

#endif
