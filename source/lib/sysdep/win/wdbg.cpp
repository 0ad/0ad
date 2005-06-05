// stack trace, improved assert and exception handler for Win32
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
#define _NO_CVCONST_H	// request SymTagEnum be defined
#include "dbghelp.h"
#include <OAIdl.h>	// VARIANT
#include "posix.h"

// optional: enables translation of the "unhandled exception" dialog.
#ifdef I18N
#include "ps/i18n.h"
#endif

#include "wdbg.h"
#include "assert_dlg.h"


#ifdef _MSC_VER
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "oleaut32.lib")	// VariantChangeType
#endif


// automatic module init (before main) and shutdown (before termination)
#pragma data_seg(".LIB$WCC")
WIN_REGISTER_FUNC(wdbg_init);
#pragma data_seg(".LIB$WTB")
WIN_REGISTER_FUNC(wdbg_shutdown);
#pragma data_seg()


// debug_warn usually uses assert2, but we don't want to call that from
// inside an assert2 (from inside another assert2 (from inside another assert2
// (... etc))), so just use the normal assert
#undef debug_warn
#define debug_warn(str) assert(0 && (str))


// protects dbghelp (which isn't thread-safe) and
// parameter passing to the breakpoint helper thread.
static void lock()
{
	win_lock(WDBG_CS);
}

static void unlock()
{
	win_unlock(WDBG_CS);
}


enum WdbgError
{
	WDBG_UNRETRIEVABLE_STATIC = -100000
};


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

	OutputDebugString(buf);
}


void debug_wprintf(const wchar_t* fmt, ...)
{
	wchar_t buf[MAX_CNT];
	buf[MAX_CNT-1] = L'\0';

	va_list ap;
	va_start(ap, fmt);
	vsnwprintf(buf, MAX_CNT-1, fmt, ap);
	va_end(ap);

	OutputDebugStringW(buf);
}


void debug_check_heap()
{
	__try
	{
		_heapchk();
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}


//////////////////////////////////////////////////////////////////////////////
//
// dbghelp symbol engine
//
//////////////////////////////////////////////////////////////////////////////



// passed to all dbghelp symbol query functions. we're not interested in
// resolving symbols in other processes; the purpose here is only to
// generate a stack trace. if that changes, we need to init a local copy
// of these in dump_sym_cb and pass them to all subsequent dump_*.
static HANDLE hProcess;
static ULONG64 mod_base;

// for StackWalk64; taken from PE header by wdbg_init
static WORD machine;

static int sym_init()
{
	hProcess = GetCurrentProcess();

	SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_DEBUG);
	// loads symbols for all active modules.
	BOOL ok = SymInitialize(hProcess, 0, TRUE);
	if(!ok)
		display_msg("wdbg_init", "SymInitialize failed");

	mod_base = SymGetModuleBase64(hProcess, (u64)&wdbg_init);
	IMAGE_NT_HEADERS* header = ImageNtHeader((void*)mod_base);
	machine = header->FileHeader.Machine;

	return 0;
}


static int sym_shutdown()
{
	SymCleanup(hProcess);
	return 0;
}


// ~500µs
int debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line)
{
	const DWORD64 addr = (DWORD64)ptr_of_interest;
	int successes = 0;

	lock();

	// get symbol name
	if(sym_name)
	{
		sym_name[0] = '\0';
		SYMBOL_INFO_PACKAGE sp;
		SYMBOL_INFO* sym = &sp.si;
		sym->SizeOfStruct = sizeof(sp.si);
		sym->MaxNameLen = MAX_SYM_NAME;
		if(SymFromAddr(hProcess, addr, 0, sym))
		{
			snprintf(sym_name, DBG_SYMBOL_LEN, "%s", sym->Name);
			successes++;
		}
	}

	// get source file + line number
	if(file || line)
	{
		IMAGEHLP_LINE64 line_info = { sizeof(IMAGEHLP_LINE64) };
		DWORD displacement; // unused but required by SymGetLineFromAddr64!
		if(SymGetLineFromAddr64(hProcess, addr, &displacement, &line_info))
			successes++;
		// note: were left zeroed if SymGetLineFromAddr64 failed
		if(file)
			snprintf(file, DBG_FILE_LEN, "%s", line_info.FileName);
		if(line)
			*line = line_info.LineNumber;
	}

	unlock();
	return (successes == 0)? -1 : 0;
}


//////////////////////////////////////////////////////////////////////////////


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
// stack walk via dbghelp
//
//////////////////////////////////////////////////////////////////////////////

// rationale: to function properly, StackWalk64 requires a CONTEXT on
// non-x86 systems (documented) or when in release mode (observed).
// exception handlers can call walk_stack with their context record;
// otherwise (e.g. dump_stack from assert2), we need to query it.
// there are 2 platform-independent ways to do so:
// - intentionally raise an SEH exception, then proceed as above;
// - GetThreadContext while suspended (*).
// the latter is more complicated and slower, so we go with the former
// despite it outputting "first chance exception" on each call.
//
// on IA-32, we use ia32_get_win_context instead of the above because
// it is 100% accurate (noticeable in StackWalk64 results) and simplest.
//
// * it used to be common practice not to query the current thread's context,
// but WinXP SP2 and above require it be suspended.

// copy from CONTEXT to STACKFRAME64
#if defined(_M_AMD64)
# define PC_ Rip
# define FP_ Rbp
# define SP_ Rsp
#elif defined(_M_IX86)
# define PC_ Eip
# define FP_ Ebp
# define SP_ Esp
#endif

#ifdef _M_IX86

// optimized for size.
static __declspec(naked) void __cdecl fill_context(void* pcontext)
{
__asm
{
	pushad
	pushfd
	mov		edi, [esp+4+32+4]	;// pcontext

	;// ContextFlags
	mov		eax, 0x10007		;// segs, int, control
	stosd

	;// DRx and FloatSave
	;// rationale: we can't access the debug registers from Ring3, and
	;// the FPU save area is irrelevant, so zero them.
	xor		eax, eax
	push	6+8+20
	pop		ecx
rep	stosd

	;// CONTEXT_SEGMENTS
	mov		ax, gs
	stosd
	mov		ax, fs
	stosd
	mov		ax, es
	stosd
	mov		ax, ds
	stosd

	;// CONTEXT_INTEGER
	mov		eax, [esp+4+32-32]	;// edi
	stosd
	xchg	eax, esi
	stosd
	xchg	eax, ebx
	stosd
	xchg	eax, edx
	stosd
	mov		eax, [esp+4+32-8]	;// ecx
	stosd
	mov		eax, [esp+4+32-4]	;// eax
	stosd

	;// CONTEXT_CONTROL		
	xchg	eax, ebp
	stosd
	mov		eax, [esp+4+32]		;// eip
	sub		eax, 5				;// back up to call site from ret addr
	stosd
	xor		eax, eax
	mov		ax, cs
	stosd
	pop		eax					;// eflags
	stosd
	lea		eax, [esp+32+4+4]	;// esp
	stosd
	xor		eax, eax
	mov		ax, ss
	stosd

	;// ExtendedRegisters
	push	512/4
	pop		ecx
rep	stosd

	popad
	ret
}
}

#else	// #ifdef _M_IX86

static void fill_context(CONTEXT* pcontext)
{
	__try
	{
		RaiseException(0xF00L, 0, 0, 0);
	}
	__except(*pcontext = (GetExceptionInformation())->ContextRecord, EXCEPTION_CONTINUE_EXECUTION)
	{
		assert(0);	// never reached
	}
}

#endif




// called for each stack frame found by walk_stack, passing information
// about the frame and <user_arg>.
// return <= 0 to stop immediately and have walk_stack return that;
// otherwise, > 0 to continue.
//
// rationale: we can't just pass function's address to the callback -
// dump_frame_cb needs the frame pointer for reg-relative variables.
typedef int (*StackFrameCallback)(const STACKFRAME64*, void*);

// iterate over a call stack, calling back for each frame encountered.
// if <pcontext> != 0, we start there; otherwise, at the current context.
// return -1 if callback never succeeded (returned 0).
static int walk_stack(StackFrameCallback cb, void* user_arg = 0, uint skip = 0, const CONTEXT* pcontext = 0)
{
	const HANDLE hThread = GetCurrentThread();

	CONTEXT context;
	if(!pcontext)
	{
		fill_context(&context);
		pcontext = &context;

		skip++;	// skip walk_stack's frame
	}

	STACKFRAME64 sf;
	memset(&sf, 0, sizeof(sf));
	sf.AddrPC.Offset    = pcontext->PC_;
	sf.AddrPC.Mode      = AddrModeFlat;
	sf.AddrFrame.Offset = pcontext->FP_;
	sf.AddrFrame.Mode   = AddrModeFlat;
	sf.AddrStack.Offset = pcontext->SP_;
	sf.AddrStack.Mode   = AddrModeFlat;

	// StackWalk64 may write to pcontext, but there's no mention of
	// EXCEPTION_POINTERS.ContextRecord being read-only, so don't copy it.

	// for each stack frame found:
	for(;;)
	{
		lock();
		BOOL ok = StackWalk64(machine, hProcess, hThread, &sf, (void*)pcontext,
			0, SymFunctionTableAccess64, SymGetModuleBase64, 0);
		unlock();

		// callback never indicated success and no (more) frames found: abort.
		// note: also test FP because StackWalk64 sometimes erroneously
		// reports success. unfortunately it doesn't SetLastError either,
		// so we can't indicate the cause of failure. *sigh*
		if(!ok || !sf.AddrFrame.Offset)
			return -911;	// distinctive error value

		if(skip)
		{
			skip--;
			continue;
		}

		int ret = cb(&sf, user_arg);
		// callback reports it's done; stop calling it and return that value.
		// (can be 0 for success, or a negative error code)
		if(ret <= 0)
			return ret;
	}
}


//
// get address of Nth function above us on the call stack (uses walk_stack)
//

// called by walk_stack for each stack frame
static int nth_caller_cb(const STACKFRAME64* sf, void* user_arg)
{
	void** pfunc = (void**)user_arg;

	// return its address
	*pfunc = (void*)sf->AddrPC.Offset;
	return 0;
}


// n starts at 1
void* debug_get_nth_caller(uint n)
{
	void* func;	// set by callback
	const uint skip = n-1 + 3;
		// make 0-based; skip walk_stack, debug_get_nth_caller and its caller.
	if(walk_stack(nth_caller_cb, &func, skip) == 0)
		return func;
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// helper routines for symbol value dump
//
//////////////////////////////////////////////////////////////////////////////

// overflow is impossible in practice. keep in sync with DumpState.
static const uint MAX_INDIRECTION = 256;
static const uint MAX_LEVEL = 256;

struct DumpState
{
	// keep in sync with MAX_* above
	uint level : 8;
	uint indirection : 8;
	uint fits_on_one_line : 1;

	DumpState()
	{
		level = 0;
		indirection = 0;
		fits_on_one_line = 0;
	}
};

static const size_t DUMP_BUF_SIZE = 64*KiB;
static wchar_t dump_buf[DUMP_BUF_SIZE];
static wchar_t* dump_buf_pos;

static void out(const wchar_t* fmt, ...)
{
	// Don't overflow the buffer (and abort if we're about to)
	if (dump_buf_pos-dump_buf+1000 > DUMP_BUF_SIZE)
	{
		debug_warn("out: buffer about to overflow");
		return;
	};

	va_list args;
	va_start(args, fmt);
	dump_buf_pos += vswprintf(dump_buf_pos, 1000, fmt, args);
	va_end(args);
}


static void out_erase(size_t num_chars)
{
	dump_buf_pos -= num_chars;
	assert2(dump_buf_pos >= dump_buf);	// check for underrun
	*dump_buf_pos = '\0';
		// make sure it's 0-terminated in case there is no further output.
}


static void out_reset()
{
	dump_buf_pos = dump_buf;
}


#define INDENT STMT(for(uint i = 0; i <= state.level+1; i++) out(L"    ");)


// does it look like an ASCII string is located at <addr>?
// set <stride> to 2 to search for WCS-2 strings (of western characters!).
// called by dump_sequence for its string special-case.
//
// algorithm: scan the "string" and count # text chars vs. garbage.
static bool is_string(const u8* p, size_t stride)
{
	// note: access violations are caught by dump_sym; output is "?".
	int score = 0;
	for(;;)
	{
		// current character is:
		const int c = *p & 0xff;	// prevent sign extension
		p += stride;
		// .. text
		if(isalnum(c))
			score += 5;
		// .. end of string
		else if(!c)
			break;
		// .. garbage
		else if(!isprint(c))
			score -= 4;

		// got enough information either way => done.
		// (we don't want to unnecessarily scan huge binary arrays)
		if(abs(score) >= 10)
			break;
	}

	return (score > 0);
}


static bool is_bogus_pointer(const void* p)
{
#ifdef _M_IX86
	if(p < (void*)0x10000)
		return true;
	if(p >= (void*)(uintptr_t)0x80000000)
		return true;
#endif

	return IsBadReadPtr(p, 1) != 0;
}


// provide c_str() access for any specialization of std::basic_string
// (since dump_string doesn't know type at compile-time).
// also performs a basic sanity check to see if the object is initialized.
struct AnyString : public std::string
{
	const void* safe_c_str(size_t el_size) const
	{
		// bogus
		if(_Myres < _Mysize)
			return 0;
		return (_Myres < 16/el_size)? _Bx._Buf : _Bx._Ptr;
	}
};

static int dump_string(WCHAR* type_name, const u8* p, size_t size, DumpState state)
{
	size_t el_size;
	const WCHAR* pretty_name = type_name;
	const void* string_data = 0;

	// Pyrogenesis CStr
	if(!wcsncmp(type_name, L"CStr", 4))
	{
		assert(size == 32/*sizeof(CStr)*/);

		// determine type
		if(type_name[4] == '8')
			el_size = sizeof(char);
		else if(type_name[4] == 'W')
			el_size = sizeof(wchar_t);
		// .. unknown, shouldn't handle it
		else
			return 1;

		p += 4;	// skip vptr (mixed in by ISerializable)
		string_data = ((AnyString*)p)->safe_c_str(el_size);
	}
	// std::basic_string and its specializations
	else if(!wcsncmp(type_name, L"std::basic_string", 17))
	{
		assert(size == sizeof(std::string) || size == 16);
			// dbghelp bug: std::wstring size is given as 16

		// determine type
		if(!wcsncmp(type_name+18, L"char", 4))
		{
			el_size = sizeof(char);
			pretty_name = L"std::string";
		}
		else if(!wcsncmp(type_name+18, L"unsigned short", 14))
		{
			el_size = sizeof(wchar_t);
			pretty_name = L"std::wstring";
		}
		// .. unknown, shouldn't handle it
		else
			return 1;

		string_data = ((AnyString*)p)->safe_c_str(el_size);
	}
	// type_name isn't a known string object; we can't handle it.
	else
		return 1;

	// type_name is known but its contents are bogus; so indicate.
	if(is_bogus_pointer(string_data) ||	!is_string((const u8*)string_data, el_size))
		out(L"(uninitialized/invalid %s)", pretty_name);
	// valid; display it.
	else
	{
		const wchar_t* fmt = (el_size == sizeof(wchar_t))? L"\"%s\"" : L"\"%hs\"";
		out(fmt, string_data);
	}

	// it was a string object (valid or not) -> we handled it.
	return 0;
}


static bool should_suppress_udt(WCHAR* type_name)
{
	// STL
	if(!wcsncmp(type_name, L"std::", 5))
		return true;

	// specialized HANDLEs are defined as pointers to structs by
	// DECLARE_HANDLE. we only want the numerical value (pointer address),
	// so prevent these structs from being displayed.
	// note: no need to check for indirection; these are only found in
	// HANDLEs (which are pointers).
	// removed obsolete defs: HEVENT, HFILE, HUMPD
	if(type_name[0] != 'H')
		goto not_handle;
#define SUPPRESS_HANDLE(name) if(!wcscmp(type_name, L#name L"__")) return true;
	SUPPRESS_HANDLE(HACCEL);
	SUPPRESS_HANDLE(HBITMAP);
	SUPPRESS_HANDLE(HBRUSH);
	SUPPRESS_HANDLE(HCOLORSPACE);
	SUPPRESS_HANDLE(HCURSOR);
	SUPPRESS_HANDLE(HDC);
	SUPPRESS_HANDLE(HENHMETAFILE);
	SUPPRESS_HANDLE(HFONT);
	SUPPRESS_HANDLE(HGDIOBJ);
	SUPPRESS_HANDLE(HGLOBAL);
	SUPPRESS_HANDLE(HGLRC);
	SUPPRESS_HANDLE(HHOOK);
	SUPPRESS_HANDLE(HICON);
	SUPPRESS_HANDLE(HIMAGELIST);
	SUPPRESS_HANDLE(HIMC);
	SUPPRESS_HANDLE(HINSTANCE);
	SUPPRESS_HANDLE(HKEY);
	SUPPRESS_HANDLE(HKL);
	SUPPRESS_HANDLE(HKLOCAL);
	SUPPRESS_HANDLE(HMENU);
	SUPPRESS_HANDLE(HMETAFILE);
	SUPPRESS_HANDLE(HMODULE);
	SUPPRESS_HANDLE(HMONITOR);
	SUPPRESS_HANDLE(HPALETTE);
	SUPPRESS_HANDLE(HPEN);
	SUPPRESS_HANDLE(HRGN);
	SUPPRESS_HANDLE(HRSRC);
	SUPPRESS_HANDLE(HSTR);
	SUPPRESS_HANDLE(HTASK);
	SUPPRESS_HANDLE(HWINEVENTHOOK);
	SUPPRESS_HANDLE(HWINSTA);
	SUPPRESS_HANDLE(HWND);
not_handle:

	return false;
}


// forward decl; called by dump_sequence and some of dump_sym_*.
static int dump_sym(DWORD idx, const u8* p, DumpState state);


static int dump_sequence(const u8* p, uint num_elements, DWORD el_idx, size_t el_size, DumpState state)
{
	// special case for character arrays: display as string
	if(el_size == sizeof(char) || el_size == sizeof(wchar_t))
		if(is_string(p, el_size))
		{
			// make sure it's 0-terminated
			wchar_t buf[512];
			if(el_size == sizeof(wchar_t))
				wcscpy_s(buf, ARRAY_SIZE(buf), (const wchar_t*)p);
			else
			{
				size_t i;
				for(i = 0; i < ARRAY_SIZE(buf)-1; i++)
				{
					buf[i] = (wchar_t)p[i];
					if(buf[i] == '\0')
						break;
				}
				buf[i] = '\0';
			}
	
			out(L"\"%s\"", buf);
			return 0;
		}

	// regular array:
	const uint num_elements_to_show = MIN(20, num_elements);
	const bool fits_on_one_line =
		(el_size == sizeof(char) && num_elements <= 16) ||
		(el_size <= sizeof(int ) && num_elements <=  8);

	state.level++;
	state.fits_on_one_line = fits_on_one_line;

	out(fits_on_one_line? L"{ " : L"\r\n");

	int err = 0;
	for(uint i = 0; i < num_elements_to_show; i++)
	{
		int ret = dump_sym(el_idx, p + i*el_size, state);
		if(err == 0)	// remember first error
			err = ret;

		// add separator unless this is the last element
		// (can't just erase below due to additional "...")
		if(i != num_elements_to_show-1 && fits_on_one_line)
			out(L", ");
	}
	// we truncated some
	if(num_elements != num_elements_to_show)
		out(L" ...");

	if(fits_on_one_line)
		out(L" }");
	return err;
}


static int determine_symbol_address(DWORD idx, const u8** pp, size_t size)
{
	DWORD data_kind;
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_DATAKIND, &data_kind))
	{
SGTI_FAILED:
		debug_warn("determine_symbol_address: SymGetTypeInfo failed");
		return -1;
	}

	DWORD ofs = 0;
	ULONG64 addr = 0;

/*
SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_ADDRESSOFFSET, &ofs);
SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_ADDRESS, &addr);
debug_printf("det_sym_addr: dk=%d addr=%I64p ofs=%d\n", data_kind, addr, ofs);
*/

	switch(data_kind)
	{
	// plain variables: p is already correct
	case DataIsLocal:
	case DataIsParam:

	case DataIsGlobal:
	case DataIsStaticLocal:
	case DataIsFileStatic:

	case DataIsObjectPtr:
		break;

	// UDT member: get offset
	case DataIsMember:
		if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_OFFSET, &ofs))
			goto SGTI_FAILED;
		assert(!size || ofs < size);
		*pp += ofs;
		break;

	// note: sometimes erroneously reported, but there's nothing we can do
	// because TI_GET_ADDRESS returns mod_base, TI_GET_ADDRESSOFFSET 0,
	// and TI_GET_OFFSET fails (it's only for members).
	case DataIsStaticMember:
		return WDBG_UNRETRIEVABLE_STATIC;

	default:
		debug_warn("dump_sym_data: invalid data kind");
		return -1;
	}

	// success
	return 0;
}



//////////////////////////////////////////////////////////////////////////////
//
// dump routines for each dbghelp symbol type
//
//////////////////////////////////////////////////////////////////////////////

// these functions return -1 if they're not able to produce any reasonable
// output; dump_data_sym will display value as "?"
// called by dump_sym; lock is held.


static int dump_sym_array(DWORD idx, const u8* p, size_t size, DumpState state)
{
	// get element count and size
	DWORD el_idx = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_TYPEID, &el_idx))
		return -1;
	// .. workaround: TI_GET_COUNT returns total struct size for
	//    arrays-of-struct. therefore, calculate as size / el_size.
	ULONG64 el_size_;
	if(!SymGetTypeInfo(hProcess, mod_base, el_idx, TI_GET_LENGTH, &el_size_))
		return -1;
	const size_t el_size = (size_t)el_size_;
	assert(el_size != 0);
	const uint num_elements = (uint)(size / el_size);
	assert2(num_elements != 0);

	// display element count
	out_erase(3);	// " = "
	out(L"[%d] = ", num_elements);

	return dump_sequence(p, num_elements, el_idx, el_size, state);
}


//////////////////////////////////////////////////////////////////////////////


static int dump_sym_base_type(DWORD idx, const u8* p, size_t size, DumpState state)
{
	DWORD base_type;
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_BASETYPE, &base_type))
		return -1;

	u64 data = movzx_64le(p, size);
	// if value is 0xCC..CC (uninitialized mem), we display as hex. 
	// the output would otherwise be garbage; this makes it obvious.
	// note: be very careful to correctly handle size=0 (e.g. void*).
	for(size_t i = 0; i < size; i++)
	{
		if(p[i] != 0xCC)
			break;
		if(i == size-1)
			goto uninitialized;
	}

	// single out() call. note: we pass a single u64 for all sizes,
	// which will only work on little-endian systems.
	const wchar_t* fmt;

	switch(base_type)
	{
		// boolean
		case btBool:
			assert(size == sizeof(bool));
			fmt = L"%hs";
			data = (u64)(data? "true " : "false");
			break;

		// floating-point
		case btFloat:
			if(size == sizeof(float))
				fmt = L"%g";
			else if(size == sizeof(double))
				fmt = L"%lg";
			else
				debug_warn("dump_sym_base_type: invalid float size");
			break;

		// signed integers (displayed as decimal)
		case btInt:
		case btLong:
			if(size == 1 || size == 2 || size == 4 || size == 8)
				fmt = L"%I64d";
			else
				debug_warn("dump_sym_base_type: invalid int size");
			break;

		// unsigned integers (displayed as hex)
		// note: 0x00000000 can get annoying (0 would be nicer),
		// but it indicates the variable size and makes for consistently
		// formatted structs/arrays. (0x1234 0 0x5678 is ugly)
		case btUInt:
		case btULong:
uninitialized:
			if(size == 1)
			{
				// _TUCHAR
				if(state.indirection)
				{
					state.indirection = 0;
					return dump_sequence(p, 8, idx, size, state);
				}
				fmt = L"0x%02X";
			}
			else if(size == 2)
				fmt = L"0x%04X";
			else if(size == 4)
				fmt = L"0x%08X";
			else if(size == 8)
				fmt = L"0x%016I64X";
			else
				debug_warn("dump_sym_base_type: invalid uint size");
			break;

		// character
		case btChar:
		case btWChar:
			assert(size == sizeof(char) || size == sizeof(wchar_t));
			// char*, wchar_t*
			if(state.indirection)
			{
				state.indirection = 0;
				return dump_sequence(p, 8, idx, size, state);
			}
			// either integer or character;
			// if printable, the character will be appended below.
			fmt = L"%d";
			break;

		// note: void* is sometimes indicated as (pointer, btNoType).
		case btVoid:
		case btNoType:
			// void* - cannot display what it's pointing to (type unknown).
			if(state.indirection)
			{
				out_erase(4);	// " -> "
				fmt = L"";
			}
			else
				debug_warn("dump_sym_base_type: non-pointer btVoid or btNoType");
			break;

		default:
			debug_warn("dump_sym_base_type: unknown type");
			//-fallthrough

		// unsupported complex types
		case btBCD:
		case btCurrency:
		case btDate:
		case btVariant:
		case btComplex:
		case btBit:
		case btBSTR:
		case btHresult:
			return -1;
	}

	out(fmt, data);

	// if the current value is a printable character, display in that form.
	// this isn't only done in btChar because sometimes ints store characters.
	if(data < 0x100)
	{
		int c = (int)data;
		if(isprint(c))
			out(L" ('%hc')", c);
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////


static int dump_sym_base_class(DWORD idx, const u8* p, size_t size, DumpState state)
{
	// unsupported: virtual base classes would require reading the VTbl,
	// which is difficult given lack of documentation and not worth it.
	return 0;
}


//////////////////////////////////////////////////////////////////////////////


static int dump_sym_data(DWORD idx, const u8* p, size_t size, DumpState state)
{
	// SymFromIndexW will fail if dataKind happens to be DataIsMember, so
	// we use SymGetTypeInfo (slower and less convenient, but no choice).
	DWORD type_idx;
	WCHAR* name;
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_TYPEID, &type_idx))
	{
SGTI_FAILED:
		debug_warn("dump_sym_data: SymGetTypeInfo failed");
		return -1;
	}
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_SYMNAME, &name))
		goto SGTI_FAILED;


	if(!state.fits_on_one_line)
		INDENT;
	out(L"%s = ", name);
	LocalFree(name);

	int err;
	__try
	{
		err = determine_symbol_address(idx, &p, size);
		if(err == 0)
			err = dump_sym(type_idx, p, state);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		out(L"(internal error)\r\n");
		err = -1;
	}
	// .. dbghelp flaw; see above.
	if(err == WDBG_UNRETRIEVABLE_STATIC)
		out(L"(unavailable - located in another module)");
	// .. failed to produce any reasonable output for whatever reason.
	else if(err < 0)
		out(L"?");

	out(state.fits_on_one_line? L", " : L"\r\n");
	return 0;
	// by aborting *for this symbol* and displaying value as "?",
	// any errors are considered handled. we don't want one faulty
	// member to prevent the entire remaining UDT from being displayed.
	// anything really serious (unknown ATM) should be special-cased.
}


//////////////////////////////////////////////////////////////////////////////


static int dump_sym_enum(DWORD idx, const u8* p, size_t size, DumpState state)
{
	const i64 current_value = movsx_64le(p, size);

	DWORD num_children;
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_CHILDRENCOUNT, &num_children))
		goto name_unavailable;

	// alloc an array to hold child IDs
	const size_t MAX_CHILDREN = 1000;
	char child_buf[sizeof(TI_FINDCHILDREN_PARAMS)+MAX_CHILDREN*sizeof(DWORD)];
	TI_FINDCHILDREN_PARAMS* fcp = (TI_FINDCHILDREN_PARAMS*)child_buf;
	fcp->Start = 0;
	fcp->Count = MIN(num_children, MAX_CHILDREN);

	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_FINDCHILDREN, fcp))
		goto name_unavailable;

	for(uint i = 0; i < fcp->Count; i++)
	{
		DWORD child_data_idx = fcp->ChildId[i];

		// get enum value. don't make any assumptions about the
		// variant's type (i.e. size)  - no restriction is documented.
		// also don't do this manually - it's tedious and we might not
		// cover everything. OLE DLL is already pulled in anyway.
		VARIANT v;
		SymGetTypeInfo(hProcess, mod_base, child_data_idx, TI_GET_VALUE, &v);
		if(VariantChangeType(&v, &v, 0, VT_I8) != S_OK)
			continue;

		if(current_value == v.llVal)
		{
			WCHAR* name;
			if(!SymGetTypeInfo(hProcess, mod_base, child_data_idx, TI_GET_SYMNAME, &name))
				goto name_unavailable;

			out(L"%s", name);
			LocalFree(name);
			return 0;
		}
	}

name_unavailable:
	// we can produce reasonable output (the numeric value),
	// but weren't able to retrieve the matching enum name.
	out(L"%I64d", current_value);
	return 1;
}


//////////////////////////////////////////////////////////////////////////////


static int dump_sym_function(DWORD idx, const u8* p, size_t size, DumpState state)
{
	return 0;
}


//////////////////////////////////////////////////////////////////////////////


static int dump_sym_function_type(DWORD idx, const u8* p, size_t size, DumpState state)
{
	// this symbol gives class parent, return type, and parameter count.
	// unfortunately the one thing we care about, its name,
	// isn't exposed via TI_GET_SYMNAME, so we resolve it ourselves.

	unlock();	// prevent recursive lock

	char name[DBG_SYMBOL_LEN];
	int err = debug_resolve_symbol((void*)p, name, 0, 0);

	lock();

	out(L"0x%p", p);
	if(err == 0)
		out(L" (%hs)", name);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////


static int dump_sym_pointer(DWORD idx, const u8* p, size_t size, DumpState state)
{
	// read+output pointer's value.
	p = (const u8*)movzx_64le(p, size);
	out(L"0x%p", p);

	// bail if it's obvious the pointer is bogus
	// (=> can't display what it's pointing to)
	if(is_bogus_pointer(p))
		return 0;

	// display what the pointer is pointing to. if the pointer is invalid
	// (despite "bogus" check above), dump_sym recovers via SEH and
	// returns -1; dump_sym_data will print "?"
	out(L" -> "); // we out_erase this if it's a void* pointer
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_TYPEID, &idx))
		return -1;
	state.indirection++;
	return dump_sym(idx, p, state);
}


//////////////////////////////////////////////////////////////////////////////


static int dump_sym_typedef(DWORD idx, const u8* p, size_t size, DumpState state)
{
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_TYPEID, &idx))
		return -1;
	return dump_sym(idx, p, state);
}


//////////////////////////////////////////////////////////////////////////////


static int dump_sym_udt(DWORD idx, const u8* p, size_t size, DumpState state)
{
	if(state.level >= 30)
	{
		out(L"level > 100 - aborting");
		return 0;
	}

	// handle special cases (e.g. HANDLE, std::string).
	WCHAR* type_name;
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_SYMNAME, &type_name))
		debug_warn("dump_sym_udt: TI_GET_SYMNAME failed");
	else
	{
		bool handled = false;
		if(dump_string(type_name, p, size, state) == 0)
			handled = true;
		else if(should_suppress_udt(type_name))
		{
			// the data symbol is pointer-to-UDT. since we won't display its
			// contents, leave only the pointer's value.
			if(state.indirection)
				out_erase(4);	// " -> "
			handled = true;
		}

//if(!handled)
//debug_wprintf(L"UDT %s: ", type_name);
		LocalFree(type_name);

		if(handled)
			return 0;	// done
	}

	// get array of child symbols (members/functions/base classes).
	DWORD num_children;
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_CHILDRENCOUNT, &num_children))
		return -1;

//debug_printf("children=%d\n", num_children);

if(!num_children)
{
out(L"{empty}");
return 0;
}


	const size_t MAX_CHILDREN = 500;
	char child_buf[sizeof(TI_FINDCHILDREN_PARAMS)+MAX_CHILDREN*sizeof(DWORD)];
	TI_FINDCHILDREN_PARAMS* fcp = (TI_FINDCHILDREN_PARAMS*)child_buf;
	fcp->Start = 0;
	fcp->Count = MIN(num_children, MAX_CHILDREN);
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_FINDCHILDREN, fcp))
		return -1;

	const size_t avg_size = size / MAX(fcp->Count, 1);	// prevent / 0
		// if num_children ends up large (e.g. due to member functions),
		// avg_size is 0. fits_on_one_line will then be false anyway.
	const bool fits_on_one_line = (fcp->Count <= 3) && (avg_size <= sizeof(int));

	out(fits_on_one_line? L"{ " : L"\r\n");

	// recursively display each child
	state.level++;
	state.fits_on_one_line = fits_on_one_line;
	int err = 0;
	for(uint i = 0; i < fcp->Count; i++)
	{
		int ret = dump_sym(fcp->ChildId[i], p, state);
		if(err == 0)
			err = ret;
	}

	if(fits_on_one_line)
	{
		// note: can't avoid writing this by checking if i == fcp->Count-1:
		// each child might be the last valid data member.
		out_erase(2);	// ", "
		out(L" }");
	}

	return err;
}


//////////////////////////////////////////////////////////////////////////////


static int dump_sym_unknown(DWORD idx, const u8* p, size_t size, DumpState state)
{
	// redundant (already done in dump_sym), but this is rare.
	DWORD type_tag;
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_SYMTAG, &type_tag))
	{
		debug_warn("dump_sym_unknown: tag query failed");
		return -1;
	}
	debug_printf("Unknown tag: %d\n", type_tag);

	out(L"(unknown symbol type)");
	return 0;
}


//////////////////////////////////////////////////////////////////////////////


// write name and value of the symbol <idx> to the output buffer.
// delegates to dump_sym_* depending on the symbol's tag.
static int dump_sym(DWORD idx, const u8* p, DumpState state)
{
	DWORD type_tag;
	if(!SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_SYMTAG, &type_tag))
		return -1;

	// get symbol size. half of dump_sym_* need this, so we query it
	// here and relay to all of them. this property isn't defined for
	// all symbol types (e.g. function), so ignore failures.
	ULONG64 size_ = 0;
	SymGetTypeInfo(hProcess, mod_base, idx, TI_GET_LENGTH, &size_);
	const size_t size = (size_t)size_;

	switch(type_tag)
	{
	case SymTagArrayType:
		return dump_sym_array         (idx, p, size, state);
	case SymTagBaseType:
		return dump_sym_base_type     (idx, p, size, state);
	case SymTagBaseClass:
		return dump_sym_base_class    (idx, p, size, state);
	case SymTagData:
		return dump_sym_data          (idx, p, size, state);
	case SymTagEnum:
		return dump_sym_enum          (idx, p, size, state);
	case SymTagFunction:
		return dump_sym_function      (idx, p, size, state);
	case SymTagFunctionType:
		return dump_sym_function_type (idx, p, size, state);
	case SymTagPointerType:
		return dump_sym_pointer       (idx, p, size, state);
	case SymTagTypedef:
		return dump_sym_typedef       (idx, p, size, state);
	case SymTagUDT:
		return dump_sym_udt           (idx, p, size, state);
	default:
		return dump_sym_unknown       (idx, p, size, state);
	}
}


//////////////////////////////////////////////////////////////////////////////
//
// stack trace
//
//////////////////////////////////////////////////////////////////////////////

struct DumpSymParams
{
	const STACKFRAME64* sf;
	bool locals_active;
};

// get actual address of what the symbol represents (may be relative
// to frame pointer); demarcate local/param sections; output name+value via
// dump_sym_data.
// 
// called from dump_frame_cb for each local symbol; lock is held.
static BOOL CALLBACK dump_sym_cb(SYMBOL_INFO* sym, ULONG sym_size, void* ctx)
{
	UNUSED(sym_size);
	DumpSymParams* p = (DumpSymParams*)ctx;

	assert(mod_base == sym->ModBase);

	// get address
	ULONG64 addr = sym->Address;
	// .. relative to a register; we assume it's the frame pointer,
	//    since sym->Register is undocumented.
	if(sym->Flags & SYMF_REGREL || sym->Flags & SYMF_FRAMEREL)
		addr += p->sf->AddrFrame.Offset;
	// .. in register; we can't reliably retrieve it (since undocumented)
	else if(sym->Flags & SYMF_REGISTER)
		return 1;
	// .. global variable (address already set)

	// demarcate local / parameter report sections - this is nicer than
	// printing e.g. "local" in front of each variable. we assume that
	// symbols are sorted by group (local/param); if not, we waste space
	// with redundant "locals:"/"params:" tags.
	// note that both flags can be set, so we can't combine the if pairs.
	if(sym->Flags & SYMF_PARAMETER)
	{
		if(p->locals_active)
		{
			out(L"    params:\r\n");
			p->locals_active = false;
		}
	}
	else if(sym->Flags & SYMF_LOCAL)
	{
		if(!p->locals_active)
		{
			out(L"    locals:\r\n ");
			p->locals_active = true;
		}
	}

	DumpState state;
	dump_sym(sym->Index, (const u8*)addr, state);

	return TRUE;	// continue
}


//////////////////////////////////////////////////////////////////////////////


// called by walk_stack for each stack frame
static int dump_frame_cb(const STACKFRAME64* sf, void* user_arg)
{
	UNUSED(user_arg);

	void* func = (void*)sf->AddrPC.Offset;
	// don't trace back into kernel32: we need a defined stop point,
	// or walk_stack will end up returning -1; stopping here also
	// reduces the risk of confusing the stack dump code below.
	wchar_t path[MAX_PATH];
	wchar_t* module_filename = get_module_filename(func, path);
	if(!wcscmp(module_filename, L"kernel32.dll"))
		return 0;	// done

	lock();

	char func_name[1000]; char file[100]; int line;
	if(debug_resolve_symbol(func, func_name, file, &line) == 0)
		out(L"%hs (%hs:%lu)", func_name, file, line);
	else
		out(L"%p", func);

	out(L"\r\n");

	// only enumerate symbols for this stack frame
	// (i.e. its locals and parameters)
	// problem: debug info is scope-aware, so we won't see any variables
	// declared in sub-blocks. we'd have to pass an address in that block,
	// which isn't worth the trouble. since 
	IMAGEHLP_STACK_FRAME imghlp_frame;
	imghlp_frame.InstructionOffset = (DWORD64)func;
	SymSetContext(hProcess, &imghlp_frame, 0);	// last param is ignored

	DumpSymParams params = { sf, true };
	SymEnumSymbols(hProcess, 0, 0, dump_sym_cb, &params);
		// 2nd and 3rd params indicate scope set by SymSetContext
		// should be used.

	out(L"\r\n");

	unlock();
	return 1;	// keep calling
}


// most recent <skip> stack frames will be skipped
// (we don't want to show e.g. GetThreadContext / this call)
static const wchar_t* dump_stack(uint skip, const CONTEXT* pcontext = 0)
{
	// if we don't get a CONTEXT, our function frame will be in the trace;
	// prevent it from being displayed. note: don't do this only in
	// walk_stack, because it may be called from different depths.
	if(!pcontext)
		skip++;

	int err = walk_stack(dump_frame_cb, 0, skip, pcontext);
	if(err != 0)
		out(L"(error while building stack trace: %d)", err);
	return dump_buf;
}


//////////////////////////////////////////////////////////////////////////////
//
// "program error" dialog (triggered by assert and exception)
//
//////////////////////////////////////////////////////////////////////////////

enum DialogType
{
	ASSERT,
	EXCEPTION
};


//
// support for resizing the dialog / its controls
// (have to do this manually - grr)
//

static POINTS dlg_client_origin;
static POINTS dlg_prev_client_size;

const int ANCHOR_LEFT   = 0x01;
const int ANCHOR_RIGHT  = 0x02;
const int ANCHOR_TOP    = 0x04;
const int ANCHOR_BOTTOM = 0x08;
const int ANCHOR_ALL    = 0x0f;

static void dlg_resize_control(HWND hDlg, int dlg_item, int dx,int dy, int anchors)
{
	HWND hControl = GetDlgItem(hDlg, dlg_item);
	RECT r;
	GetWindowRect(hControl, &r);

	int w = r.right - r.left, h = r.bottom - r.top;
	int x = r.left - dlg_client_origin.x, y = r.top - dlg_client_origin.y;

	if(anchors & ANCHOR_RIGHT)
	{
		// right only
		if(!(anchors & ANCHOR_LEFT))
			x += dx;
		// horizontal (stretch width)
		else
			w += dx;
	}

	if(anchors & ANCHOR_BOTTOM)
	{
		// bottom only
		if(!(anchors & ANCHOR_TOP))
			y += dy;
		// vertical (stretch height)
		else
			h += dy;
	}

	SetWindowPos(hControl, 0, x,y, w,h, SWP_NOZORDER);
}


static void dlg_resize(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	// 'minimize' was clicked. we need to ignore this, otherwise
	// dx/dy would reduce some control positions to less than 0.
	// since Windows clips them, we wouldn't later be able to
	// reconstruct the previous values when 'restoring'.
	if(wParam == SIZE_MINIMIZED)
		return;

	// first call for this dialog instance. WM_MOVE hasn't been sent yet,
	// so dlg_client_origin are invalid => must not call resize_control().
	// we need to set dlg_prev_client_size for the next call before exiting.
	bool first_call = (dlg_prev_client_size.y == 0);

	POINTS dlg_client_size = MAKEPOINTS(lParam);
	int dx = dlg_client_size.x - dlg_prev_client_size.x;
	int dy = dlg_client_size.y - dlg_prev_client_size.y;
	dlg_prev_client_size = dlg_client_size;

	if(first_call)
		return;

	dlg_resize_control(hDlg, IDC_CONTINUE, dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_resize_control(hDlg, IDC_SUPPRESS, dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_resize_control(hDlg, IDC_BREAK   , dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_resize_control(hDlg, IDC_EXIT    , dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_resize_control(hDlg, IDC_COPY    , dx,dy, ANCHOR_RIGHT|ANCHOR_BOTTOM);
	dlg_resize_control(hDlg, IDC_EDIT1   , dx,dy, ANCHOR_ALL);
}


static int CALLBACK dlgproc(HWND hDlg, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
		// need to reset for new instance of dialog
		dlg_client_origin.x = dlg_client_origin.y = 0;
		dlg_prev_client_size.x = dlg_prev_client_size.y = 0;
			
		// disable inappropriate buttons
		DialogType type = (DialogType)lParam;
		if(type != ASSERT)
		{
			HWND h;
			h = GetDlgItem(hDlg, IDC_CONTINUE);
			EnableWindow(h, FALSE);
			h = GetDlgItem(hDlg, IDC_SUPPRESS);
			EnableWindow(h, FALSE);
			h = GetDlgItem(hDlg, IDC_BREAK);
			EnableWindow(h, FALSE);
		}

		SetDlgItemTextW(hDlg, IDC_EDIT1, dump_buf);
		return TRUE;	// set default keyboard focus
		}

	case WM_SYSCOMMAND:
		// close dialog if [X] is clicked (doesn't happen automatically)
		// note: lower 4 bits are reserved
		if((wParam & 0xFFF0) == SC_CLOSE)
		{
			EndDialog(hDlg, 0);
			return 0;	// processed
		}
		break;

	// return 0 if processed, otherwise break
	case WM_COMMAND:
		switch(wParam)
		{
		case IDC_COPY:
			clipboard_set(dump_buf);
			return 0;

		case IDC_CONTINUE:
			EndDialog(hDlg, ASSERT_CONTINUE);
			return 0;
		case IDC_SUPPRESS:
			EndDialog(hDlg, ASSERT_SUPPRESS);
			return 0;
		case IDC_BREAK:
			EndDialog(hDlg, ASSERT_BREAK);
			return 0;
		case IDC_EXIT:
			exit(0);
			return 0;

		default:
			break;
		}
		break;

	case WM_MOVE:
		dlg_client_origin = MAKEPOINTS(lParam);
		break;

	case WM_GETMINMAXINFO:
		{
		// we must make sure resize_control will never set negative coords -
		// Windows would clip them, and its real position would be lost.
		// restrict to a reasonable and good looking minimum size [pixels].
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize.x = 407;
		mmi->ptMinTrackSize.y = 159;	// determined experimentally
		return 0;
		}

	case WM_SIZE:
		dlg_resize(hDlg, wParam, lParam);
		break;

	default:
		break;
	}

	// we didn't process the message; caller will perform default action.
	return FALSE;
}


// show error dialog with stack trace (must be stored in dump_buf[])
// exits directly if 'exit' is clicked.
static int dialog(DialogType type)
{
	const HINSTANCE hInstance = GetModuleHandle(0);
	const HWND hParent = GetDesktopWindow();
		// we don't know if the enclosing app has a hwnd, so use the desktop.
	return (int)DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), hParent, dlgproc, (LPARAM)type);
}


static LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS* ep);

// notify the user that an assertion failed; displays a
// stack trace with local variables.
// returns one of FailedAssertUserChoice or exits the program.
int debug_assert_failed(const char* file, int line, const char* expr)
{
	out_reset();
	out(L"Assertion failed in %hs, line %d: \"%hs\"\r\n", file, line, expr);
	out(L"\r\nCall stack:\r\n\r\n");
	dump_stack(+1);	// skip the current frame (debug_assert_failed)

#if defined(SCED) && !(defined(NDEBUG)||defined(TESTING))
	// ScEd keeps running while the dialog is showing, and tends to crash before
	// there's a chance to read the assert message. So, just break immediately.
	debug_break();
#endif

	return dialog(ASSERT);

/*
__try
{
	RaiseException(0x10000, 0, 0,0);
}
__except(unhandled_exception_filter(GetExceptionInformation()))
{
}
return ASSERT_CONTINUE;
*/
}


//////////////////////////////////////////////////////////////////////////////
//
// exception handler
//
//////////////////////////////////////////////////////////////////////////////

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


// write out a "minidump" containing register and stack state; this enables
// examining the crash in a debugger. called by unhandled_exception_filter.
// heavily modified from http://www.codeproject.com/debug/XCrashReportPt3.asp
static void write_minidump(EXCEPTION_POINTERS* exception_pointers)
{
	HANDLE hFile = CreateFile("crashlog.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE)
		goto fail;

	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = GetCurrentThreadId();
	mei.ExceptionPointers = exception_pointers;
	mei.ClientPointers = FALSE;
		// exception_pointers is not in our address space.

	// note: we don't store other crashlog info within the dump file
	// (UserStreamParam), since we will need to generate a plain text file on
	// non-Windows platforms. users will just have to send us both files.

	HANDLE hProcess = GetCurrentProcess(); DWORD pid = GetCurrentProcessId();
	if(!MiniDumpWriteDump(hProcess, pid, hFile, MiniDumpNormal, &mei, 0, 0))
	{
fail:
		translate_and_display_msg(L"Error", L"Unable to generate minidump.");
	}

	CloseHandle(hFile);
}

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


// if <er> is not a C++ exception, return 0. otherwise, return a description
// of the exception type and cause (in English). uses static storage.
static const wchar_t* get_cpp_exception_description(const EXCEPTION_RECORD* er)
{
	const ULONG_PTR* const ei = er->ExceptionInformation;

	// bail if not a C++ exception (magic numbers defined in VC exsup.inc)
	if(er->ExceptionCode != 0xe06d7363 ||
	   er->NumberParameters != 3 ||
	   ei[0] != 0x19930520)
		return 0;

	// VC's C++ exception implementation stores the following:
	// ei[0] - magic number
	// ei[1] -> object that was thrown
	// ei[2] -> XInfo
	//
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
	swprintf(description, ARRAY_SIZE(description), L"Unknown exception(0x%08X)", code);
	return description;
}


// return a description of the exception <er> (in English).
// it is only valid until the next call, since static storage is used.
static const wchar_t* get_exception_description(const EXCEPTION_RECORD* er)
{
	// note: more specific than SEH, so try it first.
	const wchar_t* d = get_cpp_exception_description(er);
	if(d)
		return d;

	return get_SEH_exception_description(er);
}


// return an indication of where the exception <er> occurred (lang. neutral).
// it is only valid until the next call, since static storage is used.
static const wchar_t* get_exception_locus(const EXCEPTION_RECORD* er)
{
	void* addr = er->ExceptionAddress;

	wchar_t path[MAX_PATH];
	wchar_t* module_filename = get_module_filename(er->ExceptionAddress, path);

	char func_name[DBG_SYMBOL_LEN];
	debug_resolve_symbol(addr, func_name, 0, 0);

	static wchar_t locus[100];
	swprintf(locus, ARRAY_SIZE(locus), L"%s!%p(%hs)", module_filename, addr, func_name);
	return locus;
}


// called when an SEH exception was not caught by the app;
// provides detailed debugging information and exits.
// this overrides the normal OS "program error" dialog; see rationale below.
static LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS* ep)
{
	const EXCEPTION_RECORD* const er = ep->ExceptionRecord;

	// note: we risk infinite recursion if someone raises an SEH exception
	// from within this function. therefore, abort immediately if we've
	// already been called; the first error is the most important, anyway.
	static bool already_crashed = false;
	if(already_crashed)
		return EXCEPTION_EXECUTE_HANDLER;
	already_crashed = true;

	// build and display error message
	const wchar_t* locus       = get_exception_locus      (er);
	const wchar_t* description = get_exception_description(er);
	static const wchar_t fmt[] =
		L"Much to our regret we must report the program has encountered an error and cannot continue.\r\n"
	    L"\n"
	    L"Please let us know at http://bugs.wildfiregames.com/ and attach the crashlog.txt and crashlog.dmp files.\r\n"
	    L"\n"
		L"Details: %s at %s.";
	wchar_t text[1000];
	swprintf(text, ARRAY_SIZE(text), translate(fmt), description, locus);
	wdisplay_msg(translate(L"Problem"), text);

	// write out crash log and minidump.
	write_minidump(ep);
	out_reset();
	const wchar_t* stack_trace = dump_stack(+0, ep->ContextRecord);
	debug_write_crashlog(description, locus, stack_trace);

	// disable memory-leak reporting to avoid a flood of warnings
	// (lots of stuff will leak since we exit abnormally).
#ifdef HAVE_DEBUGALLOC
	uint flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	_CrtSetDbgFlag(flags & ~_CRTDBG_LEAK_CHECK_DF);
#endif

	// invoke the default exception handler - it calls ExitProcess for
	// most exception types.
	return EXCEPTION_EXECUTE_HANDLER;
}


// called from wdbg_init.
//
// rationale:
// we want to replace the OS "program error" dialog box because
// it is not all too helpful in debugging. to that end, there are
// 4 ways to make sure unhandled SEH exceptions are caught:
// - via WaitForDebugEvent; the app is run from a separate debugger process.
//   this complicates analysis, since the exception is in another
//   address space. also, we are basically implementing a full-featured
//   debugger - overkill.
// - wrapping all threads in __try (necessary since the handler chain
//   is in TLS) is very difficult to guarantee; it would also pollute main().
// - vectored exception handlers work across threads, but
//   are only available on WinXP (unacceptable).
// - setting the per-process unhandled exception filter does the job,
//   with the following caveat: it is never called when a debugger is active.
//   workaround: call from a regular SEH __except, e.g. wrapped around main().
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
static void set_exception_handler()
{
	void* prev_filter = SetUnhandledExceptionFilter(unhandled_exception_filter);
	if(prev_filter)
		assert2("conflict with SetUnhandledExceptionFilter. must implement chaining to previous handler");

struct Small
{
	int i1;
	int i2;
};

struct Large
{
	double d1;
	double d2;
	double d3;
	double d4;
};

Large large_array_of_large_structs[8] = { { 0.0,0.0,0.0,0.0 } };
Large small_array_of_large_structs[2] = { { 0.0,0.0,0.0,0.0 } };
Small large_array_of_small_structs[8] = { { 1,2 } };
Small small_array_of_small_structs[2] = { { 1,2 } };

	int ar1[] = { 1,2,3,4,5	};
	char ar2[] = { 't','e','s','t', 0 };

	// tests
//	__try
	{
	//assert2(0 && "test assert2");				// not exception (works when run from debugger)
	//__asm xor edx,edx __asm div edx 			// named SEH
	//RaiseException(0x87654321, 0, 0, 0);		// unknown SEH
	//throw std::bad_exception("what() is ok");	// C++
	}
//	__except(unhandled_exception_filter(GetExceptionInformation()))
	{
	}
}




static int wdbg_init()
{
	RETURN_ERR(sym_init());

	// rationale: see definition. note: unhandled_exception_filter uses the
	// dbghelp symbol engine, so it must be initialized first.
	set_exception_handler();
	return 0;
}


static int wdbg_shutdown(void)
{
	return sym_shutdown();
}
