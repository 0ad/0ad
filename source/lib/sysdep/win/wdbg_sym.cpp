/**
 * =========================================================================
 * File        : wdbg_sym.cpp
 * Project     : 0 A.D.
 * Description : Win32 stack trace and symbol engine.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2002-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"

#include <stdlib.h>
#include <stdio.h>
#include <set>

#include "lib.h"
#include "win_internal.h"
#define _NO_CVCONST_H	// request SymTagEnum be defined
#include "dbghelp.h"
#include <OAIdl.h>	// VARIANT
#include "posix.h"
#include "sysdep/cpu.h"
#include "wdbg.h"
#include "debug_stl.h"
#include "app_hooks.h"
#if CPU_IA32
# include "lib/sysdep/ia32.h"
#endif

// raises an an annoying exception, so disable unless needed
#undef SELF_TEST_ENABLED
#define SELF_TEST_ENABLED 0


#if MSC_VERSION
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "oleaut32.lib")	// VariantChangeType
#endif


// automatic module shutdown (before process termination)
#pragma data_seg(WIN_CALLBACK_POST_ATEXIT(b))
WIN_REGISTER_FUNC(wdbg_sym_shutdown);
#pragma data_seg()


// note: it is safe to use debug_assert/debug_warn/CHECK_ERR even during a
// stack trace (which is triggered by debug_assert et al. in app code) because
// nested stack traces are ignored and only the error is displayed.


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


//----------------------------------------------------------------------------
// dbghelp
//----------------------------------------------------------------------------

// passed to all dbghelp symbol query functions. we're not interested in
// resolving symbols in other processes; the purpose here is only to
// generate a stack trace. if that changes, we need to init a local copy
// of these in dump_sym_cb and pass them to all subsequent dump_*.
static HANDLE hProcess;
static ULONG64 mod_base;

// for StackWalk64; taken from PE header by wdbg_init.
static WORD machine;

// call on-demand (allows handling exceptions raised before win.cpp
// init functions are called); no effect if already initialized.
static LibError sym_init()
{
	// bail if already initialized (there's nothing to do).
	// don't use pthread_once because we need to return success/error code.
	static uintptr_t already_initialized = 0;
	if(!CAS(&already_initialized, 0, 1))
		return ERR_OK;

	hProcess = GetCurrentProcess();

	// set options
	// notes:
	// - can be done before SymInitialize; we do so in case
	//   any of the options affect it.
	// - do not set directly - that would zero any existing flags.
	DWORD opts = SymGetOptions();
	opts |= SYMOPT_DEFERRED_LOADS;	// the "fastest, most efficient way"
	//opts |= SYMOPT_DEBUG;	// lots of debug spew in output window
	SymSetOptions(opts);

	// initialize dbghelp.
	// .. request symbols from all currently active modules be loaded.
	const BOOL fInvadeProcess = TRUE;
	// .. use default *symbol* search path. we don't use this to locate
	//    our PDB file because its absolute path is stored inside the EXE.
	PCSTR UserSearchPath = 0;
	BOOL ok = SymInitialize(hProcess, UserSearchPath, fInvadeProcess);
	WARN_IF_FALSE(ok);

	mod_base = SymGetModuleBase64(hProcess, (u64)&sym_init);
	IMAGE_NT_HEADERS* header = ImageNtHeader((void*)mod_base);
	machine = header->FileHeader.Machine;

	return ERR_OK;
}


// called from wdbg_sym_shutdown.
static LibError sym_shutdown()
{
	SymCleanup(hProcess);
	return ERR_OK;
}


struct SYMBOL_INFO_PACKAGEW2 : public SYMBOL_INFO_PACKAGEW
{
	SYMBOL_INFO_PACKAGEW2()
	{
		si.SizeOfStruct = sizeof(si);
		si.MaxNameLen = MAX_SYM_NAME;
	}
};

// note: we can't derive from TI_FINDCHILDREN_PARAMS because its members
// aren't guaranteed to precede ours (although they do in practice).
struct TI_FINDCHILDREN_PARAMS2
{
	TI_FINDCHILDREN_PARAMS2(DWORD num_children)
	{
		p.Start = 0;
		p.Count = MIN(num_children, MAX_CHILDREN);
	}

	static const size_t MAX_CHILDREN = 400;
	TI_FINDCHILDREN_PARAMS p;
	DWORD additional_children[MAX_CHILDREN-1];
};


// read and return symbol information for the given address. all of the
// output parameters are optional; we pass back as much information as is
// available and desired. return 0 iff any information was successfully
// retrieved and stored.
// sym_name and file must hold at least the number of chars above;
// file is the base name only, not path (see rationale in wdbg_sym).
// the PDB implementation is rather slow (~500µs).
LibError debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line)
{
	sym_init();

	const DWORD64 addr = (DWORD64)ptr_of_interest;
	int successes = 0;

	lock();

	// get symbol name (if requested)
	if(sym_name)
	{
		sym_name[0] = '\0';

		SYMBOL_INFO_PACKAGEW2 sp;
		SYMBOL_INFOW* sym = &sp.si;
		if(SymFromAddrW(hProcess, addr, 0, sym))
		{
			snprintf(sym_name, DBG_SYMBOL_LEN, "%ws", sym->Name);
			successes++;
		}
	}

	// get source file and/or line number (if requested)
	if(file || line)
	{
		file[0] = '\0';
		*line = 0;

		IMAGEHLP_LINE64 line_info = { sizeof(IMAGEHLP_LINE64) };
		DWORD displacement; // unused but required by SymGetLineFromAddr64!
		if(SymGetLineFromAddr64(hProcess, addr, &displacement, &line_info))
		{
			if(file)
			{
				// strip full path down to base name only.
				// this loses information, but that isn't expected to be a
				// problem and is balanced by not having to do this from every
				// call site (full path is too long to display nicely).
				const char* base_name = line_info.FileName;
				const char* slash = strrchr(base_name, DIR_SEP);
				if(slash)
					base_name = slash+1;

				snprintf(file, DBG_FILE_LEN, "%s", base_name);
				successes++;
			}

			if(line)
			{
				*line = line_info.LineNumber;
				successes++;
			}
		}
	}

	unlock();
	return (successes != 0)? ERR_OK : ERR_FAIL;
}


//----------------------------------------------------------------------------
// stack walk
//----------------------------------------------------------------------------


/*
Subroutine linkage example code:

	push	param2
	push	param1
	call	func
ret_addr:
	[..]

func:
	push	ebp
	mov		ebp, esp
	sub		esp, local_size
	[..]

Stack contents (down = decreasing address)
	[param2]
	[param1]
	ret_addr
	prev_ebp         (<- current ebp points at this value)
	[local_variables]
*/


/*
	call	func1
ret1:

func1:
	push	ebp
	mov		ebp, esp
	call	func2
ret2:

func2:
	push	ebp
	mov		ebp, esp
	STARTHERE

	*/

#if CPU_IA32 && !CONFIG_OMIT_FP

static LibError ia32_walk_stack(STACKFRAME64* sf)
{
	// read previous values from STACKFRAME64
	void* prev_fp  = (void*)sf->AddrFrame .Offset;
	void* prev_ip  = (void*)sf->AddrPC    .Offset;
	void* prev_ret = (void*)sf->AddrReturn.Offset;
	if(!debug_is_stack_ptr(prev_fp))
		return ERR_11;
	if(prev_ip && !debug_is_code_ptr(prev_ip))
		return ERR_12;
	if(prev_ret && !debug_is_code_ptr(prev_ret))
		return ERR_13;

	// read stack frame
	void* fp       = ((void**)prev_fp)[0];
	void* ret_addr = ((void**)prev_fp)[1];
	if(!debug_is_stack_ptr(fp))
		return ERR_14;
	if(!debug_is_code_ptr(ret_addr))
		return ERR_15;

	void* target;
	LibError err = ia32_get_call_target(ret_addr, &target);
	RETURN_ERR(err);
	if(target)	// were able to determine it from the call instruction
		debug_assert(debug_is_code_ptr(target));

	sf->AddrFrame .Offset = (DWORD64)fp;
	sf->AddrPC    .Offset = (DWORD64)target;
	sf->AddrReturn.Offset = (DWORD64)ret_addr;

	return ERR_OK;
}

#endif	// #if CPU_IA32 && !CONFIG_OMIT_FP


// called for each stack frame found by walk_stack, passing information
// about the frame and <user_arg>.
// return INFO_CB_CONTINUE to continue, anything else to stop immediately
// and return that value to walk_stack's caller.
//
// rationale: we can't just pass function's address to the callback -
// dump_frame_cb needs the frame pointer for reg-relative variables.
typedef LibError (*StackFrameCallback)(const STACKFRAME64*, void*);

// iterate over a call stack, calling back for each frame encountered.
// if <pcontext> != 0, we start there; otherwise, at the current context.
// return an error if callback never succeeded (returned 0).
//
// lock must be held.
static LibError walk_stack(StackFrameCallback cb, void* user_arg = 0, uint skip = 0, const CONTEXT* pcontext = 0)
{
	// to function properly, StackWalk64 requires a CONTEXT on
	// non-x86 systems (documented) or when in release mode (observed).
	// exception handlers can call walk_stack with their context record;
	// otherwise (e.g. dump_stack from debug_assert), we need to query it.
	CONTEXT context;
	// .. caller knows the context (most likely from an exception);
	//    since StackWalk64 may modify it, copy to a local variable.
	if(pcontext)
		context = *pcontext;
	// .. need to determine context ourselves.
	else
	{
		skip++;	// skip this frame

		// there are 4 ways to do so, in order of preference:
		// - asm (easy to use but currently only implemented on IA32)
		// - RtlCaptureContext (only available on WinXP or above)
		// - intentionally raise an SEH exception and capture its context
		//   (spams us with "first chance exception")
		// - GetThreadContext while suspended* (a bit tricky + slow).
		//
		// * it used to be common practice to query the current thread's context,
		// but WinXP SP2 and above require it be suspended.
		//
		// this MUST be done inline and not in an external function because
		// compiler-generated prolog code trashes some registers.

#if CPU_IA32
		ia32_get_current_context(&context);
#else
		// try to import RtlCaptureContext (available on WinXP and later)
		// .. note: kernel32 is always loaded into every process, so we
		//    don't need LoadLibrary/FreeLibrary.
		HMODULE hKernel32Dll = GetModuleHandle("kernel32.dll");
		VOID (*pRtlCaptureContext)(PCONTEXT*);
		*(void**)&pRtlCaptureContext = GetProcAddress(hKernel32Dll, "RtlCaptureContext");
		if(pRtlCaptureContext)
			pRtlCaptureContext(&context);
		// not available: raise+handle an exception; grab the reported context.
		else
		{
			__try
			{
				RaiseException(0xF001, 0, 0, 0);
			}
			__except(context = (GetExceptionInformation())->ContextRecord, EXCEPTION_CONTINUE_EXECUTION)
			{
			}
		}
#endif
	}
	pcontext = &context;

	STACKFRAME64 sf;
	memset(&sf, 0, sizeof(sf));
	sf.AddrPC.Offset    = pcontext->PC_;
	sf.AddrPC.Mode      = AddrModeFlat;
	sf.AddrFrame.Offset = pcontext->FP_;
	sf.AddrFrame.Mode   = AddrModeFlat;
	sf.AddrStack.Offset = pcontext->SP_;
	sf.AddrStack.Mode   = AddrModeFlat;

	// for each stack frame found:
	LibError ret = ERR_SYM_NO_STACK_FRAMES_FOUND;
	for(;;)
	{
		// rationale:
		// - provide a separate ia32 implementation so that simple
		//   stack walks (e.g. to determine callers of malloc) do not
		//   require firing up dbghelp. that takes tens of seconds when
		//   OS symbols are installed (because symserv is wanting to access
		//   inet), which is entirely unacceptable.
		// - VC7.1 sometimes generates stack frames despite /Oy ;
		//   ia32_walk_stack may appear to work, but it isn't reliable in
		//   this case and therefore must not be used!
		// - don't switch between ia32_stack_walk and StackWalk64 when one
		//   of them fails: this needlessly complicates things. the ia32
		//   code is authoritative provided its prerequisite (FP not omitted)
		//   is met, otherwise totally unusable.
		LibError err;
#if CPU_IA32 && !CONFIG_OMIT_FP
		err = ia32_walk_stack(&sf);
#else
		sym_init();
		// note: unfortunately StackWalk64 doesn't always SetLastError,
		// so we have to reset it and check for 0. *sigh*
		SetLastError(0);
		const HANDLE hThread = GetCurrentThread();
		BOOL ok = StackWalk64(machine, hProcess, hThread, &sf, (PVOID)pcontext,
			0, SymFunctionTableAccess64, SymGetModuleBase64, 0);
		// note: don't use LibError_from_win32 because it raises a warning,
		// and this "fails" commonly (when no stack frames are left).
		err = ok? ERR_OK : ERR_FAIL;
#endif

		// no more frames found - abort. note: also test FP because
		// StackWalk64 sometimes erroneously reports success.
		void* fp = (void*)(uintptr_t)sf.AddrFrame .Offset;
		if(err != ERR_OK || !fp)
			return ret;

		if(skip)
		{
			skip--;
			continue;
		}

		ret = cb(&sf, user_arg);
		// callback reports it's done; stop calling it and return that value.
		// (can be 0 for success, or a negative error code)
		if(ret != INFO_CB_CONTINUE)
		{
			debug_assert(ret <= 0);	// shouldn't return > 0
			return ret;
		}
	}
}


//
// get address of Nth function above us on the call stack (uses walk_stack)
//

// called by walk_stack for each stack frame
static LibError nth_caller_cb(const STACKFRAME64* sf, void* user_arg)
{
	void** pfunc = (void**)user_arg;

	// return its address
	*pfunc = (void*)sf->AddrPC.Offset;
	return ERR_OK;
}


// return address of the Nth function on the call stack.
// if <context> is nonzero, it is assumed to be a platform-specific
// representation of execution state (e.g. Win32 CONTEXT) and tracing
// starts there; this is useful for exceptions.
// otherwise, tracing starts at the current stack position, and the given
// number of stack frames (i.e. functions) above the caller are skipped.
// used by mmgr to determine what function requested each allocation;
// this is fast enough to allow that.
void* debug_get_nth_caller(uint skip, void* pcontext)
{
	if(!pcontext)
		skip++;	// skip this frame

	lock();

	void* func;
	LibError err = walk_stack(nth_caller_cb, &func, skip, (const CONTEXT*)pcontext);

	unlock();
	return (err == ERR_OK)? func : 0;
}



//////////////////////////////////////////////////////////////////////////////
//
// helper routines for symbol value dump
//
//////////////////////////////////////////////////////////////////////////////

// overflow is impossible in practice, but check for robustness.
// keep in sync with DumpState.
static const uint MAX_INDIRECTION = 255;
static const uint MAX_LEVEL = 255;

struct DumpState
{
	// keep in sync with MAX_* above
	uint level : 8;
	uint indirection : 8;

	DumpState()
	{
		level = 0;
		indirection = 0;
	}
};

//----------------------------------------------------------------------------

static size_t out_chars_left;
static bool out_have_warned_of_overflow;
	// only do so once until next out_init to avoid flood of messages.
static wchar_t* out_pos;

// some top-level (*) symbols cause tons of output - so much that they may
// single-handedly overflow the buffer (e.g. pointer to a tree of huge UDTs).
// we can't have that, so there is a limit in place as to how much a
// single top-level symbol can output. after that is reached, dumping is
// aborted for that symbol but continues for the subsequent top-level symbols.
//
// this is implemented as follows: dump_sym_cb latches the current output
// position; each dump_sym (through which all symbols go) checks if the
// new position exceeds the limit and aborts if so.
// slight wrinkle: since we don't want each level of UDTs to successively
// realize the limit has been hit and display the error message, we
// return ERR_SYM_SINGLE_SYMBOL_LIMIT once and thereafter ERR_SYM_SUPPRESS_OUTPUT.
//
// * example: local variables, as opposed to child symbols in a UDT.
static wchar_t* out_latched_pos;
static bool out_have_warned_of_limit;

static void out_init(wchar_t* buf, size_t max_chars)
{
	out_pos = buf;
	out_chars_left = max_chars;
	out_have_warned_of_overflow = false;
	out_have_warned_of_limit = false;
}


static void out(const wchar_t* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int len = _vsnwprintf(out_pos, out_chars_left, fmt, args);
	va_end(args);

	// success
	if(len >= 0)
	{
		out_pos += len;
		// make sure out_chars_left remains nonnegative
		if((size_t)len > out_chars_left)
		{
			debug_warn("apparently wrote more than out_chars_left");
			len = (int)out_chars_left;
		}
		out_chars_left -= len;
	}
	// no more room left
	else
	{
		// the buffer really is full yet out_chars_left may not be 0
		// (since it isn't updated if _vsnwprintf returns -1).
		// must be set so subsequent calls don't try to squeeze stuff in.
		out_chars_left = 0;

		// write a warning into the output buffer (once) so it isn't
		// abruptly cut off (which looks like an error)
		if(!out_have_warned_of_overflow)
		{
			out_have_warned_of_overflow = true;

			// with the current out_pos / out_chars_left variables, there's
			// no way of knowing where the buffer actually ends. no matter;
			// we'll just put the warning before out_pos and eat into the
			// second newest text.
			const wchar_t text[] = L"(no more room in buffer)";
			wcscpy(out_pos-ARRAY_SIZE(text), text);	// safe
		}
	}
}


static void out_erase(size_t num_chars)
{
	// don't do anything if end of buffer was hit (prevents repeatedly
	// scribbling over the last few bytes).
	if(out_have_warned_of_overflow)
		return;

	out_chars_left += (ssize_t)num_chars;
	out_pos -= num_chars;
	*out_pos = '\0';
		// make sure it's 0-terminated in case there is no further output.
}


// (see above)
static void out_latch_pos()
{
	out_have_warned_of_limit = false;
	out_latched_pos = out_pos;
}


// (see above)
static LibError out_check_limit()
{
	if(out_have_warned_of_limit)
		return ERR_SYM_SUPPRESS_OUTPUT;	// NOWARN
	if(out_pos - out_latched_pos > 3000)	// ~30 lines
	{
		out_have_warned_of_limit = true;
		return ERR_SYM_SINGLE_SYMBOL_LIMIT;	// NOWARN
	}

	// no limit hit, proceed normally
	return ERR_OK;
}

//----------------------------------------------------------------------------

#define INDENT STMT(for(uint i = 0; i <= state.level; i++) out(L"    ");)
#define UNINDENT STMT(out_erase((state.level+1)*4);)


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




// forward decl; called by dump_sequence and some of dump_sym_*.
static LibError dump_sym(DWORD id, const u8* p, DumpState state);

// from cvconst.h
//
// rationale: we don't provide a get_register routine, since only the
// value of FP is known to dump_frame_cb (via STACKFRAME64).
// displaying variables stored in registers is out of the question;
// all we can do is display FP-relative variables.
enum CV_HREG_e
{
	CV_REG_EAX = 17,
	CV_REG_ECX = 18,
	CV_REG_EDX = 19,
	CV_REG_EBX = 20,
	CV_REG_ESP = 21,
	CV_REG_EBP = 22,
	CV_REG_ESI = 23,
	CV_REG_EDI = 24
};


static const wchar_t* string_for_register(CV_HREG_e reg)
{
	switch(reg)
	{
	case CV_REG_EAX:
		return L"eax";
	case CV_REG_ECX:
		return L"ecx";
	case CV_REG_EDX:
		return L"edx";
	case CV_REG_EBX:
		return L"ebx";
	case CV_REG_ESP:
		return L"esp";
	case CV_REG_EBP:
		return L"ebp";
	case CV_REG_ESI:
		return L"esi";
	case CV_REG_EDI:
		return L"edi";
	default:
		{
			static wchar_t buf[19];
			swprintf(buf, ARRAY_SIZE(buf), L"0x%x", reg);
			return buf;
		}
	}
}


static void dump_error(LibError err, const u8* p)
{
	switch(err)
	{
	case 0:
		// no error => no output
		break;
	case ERR_SYM_SINGLE_SYMBOL_LIMIT:
		out(L"(too much output; skipping to next top-level symbol)");
		break;
	case ERR_SYM_UNRETRIEVABLE_STATIC:
		out(L"(unavailable - located in another module)");
		break;
	case ERR_SYM_UNRETRIEVABLE_REG:
		out(L"(unavailable - stored in register %s)", string_for_register((CV_HREG_e)(uintptr_t)p));
		break;
	case ERR_SYM_TYPE_INFO_UNAVAILABLE:
		out(L"(unavailable - type info request failed (GLE=%d))", GetLastError());
		break;
	case ERR_SYM_INTERNAL_ERROR:
		out(L"(unavailable - internal error)\r\n");
		break;
	case ERR_SYM_SUPPRESS_OUTPUT:
		// not an error; do not output anything. handled by caller.
		break;
	default:
		out(L"(unavailable - unspecified error 0x%X (%d))", err, err);
		break;
	}
}


// split out of dump_sequence.
static LibError dump_string(const u8* p, size_t el_size)
{
	// not char or wchar_t string
	if(el_size != sizeof(char) && el_size != sizeof(wchar_t))
		return INFO_CANNOT_HANDLE;
	// not text
	if(!is_string(p, el_size))
		return INFO_CANNOT_HANDLE;

	wchar_t buf[512];
	if(el_size == sizeof(wchar_t))
		wcscpy_s(buf, ARRAY_SIZE(buf), (const wchar_t*)p);
	// convert to wchar_t
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
	return ERR_OK;
}


// split out of dump_sequence.
static void seq_determine_formatting(size_t el_size, size_t el_count,
	bool* fits_on_one_line, size_t* num_elements_to_show)
{
	if(el_size == sizeof(char))
	{
		*fits_on_one_line = el_count <= 16;
		*num_elements_to_show = MIN(16, el_count);
	}
	else if(el_size <= sizeof(int))
	{
		*fits_on_one_line = el_count <= 8;
		*num_elements_to_show = MIN(12, el_count);
	}
	else
	{
		*fits_on_one_line = false;
		*num_elements_to_show = MIN(8, el_count);
	}

	// make sure empty containers are displayed with [0] {}, otherwise
	// the lack of output looks like an error.
	if(!el_count)
		*fits_on_one_line = true;
}


static LibError dump_sequence(DebugIterator el_iterator, void* internal,
	size_t el_count, DWORD el_type_id, size_t el_size, DumpState state)
{
	const u8* el_p = 0;	// avoid "uninitialized" warning

	// special case: display as a string if the sequence looks to be text.
	// do this only if container isn't empty because the otherwise the
	// iterator may crash.
	if(el_count)
	{
		el_p = el_iterator(internal, el_size);

		LibError ret = dump_string(el_p, el_size);
		if(ret == ERR_OK)
			return ret;
	}

	// choose formatting based on element size and count
	bool fits_on_one_line;
	size_t num_elements_to_show;
	seq_determine_formatting(el_size, el_count, &fits_on_one_line, &num_elements_to_show);

	out(L"[%d] ", el_count);
	state.level++;
	out(fits_on_one_line? L"{ " : L"\r\n");

	for(size_t i = 0; i < num_elements_to_show; i++)
	{
		if(!fits_on_one_line)
			INDENT;

		LibError err = dump_sym(el_type_id, el_p, state);
		el_p = el_iterator(internal, el_size);

		// there was no output for this child; undo its indentation (if any),
		// skip everything below and proceed with the next child.
		if(err == ERR_SYM_SUPPRESS_OUTPUT)
		{
			if(!fits_on_one_line)
				UNINDENT;
			continue;
		}

		dump_error(err, el_p);	// nop if err == ERR_OK
		// add separator unless this is the last element (can't just
		// erase below due to additional "...").
		if(i != num_elements_to_show-1)
			out(fits_on_one_line? L", " : L"\r\n");

		if(err == ERR_SYM_SINGLE_SYMBOL_LIMIT)
			break;
	}	// for each child

	// indicate some elements were skipped
	if(el_count != num_elements_to_show)
		out(L" ...");

	state.level--;
	if(fits_on_one_line)
		out(L" }");
	return ERR_OK;
}


static const u8* array_iterator(void* internal, size_t el_size)
{
	const u8*& pos = *(const u8**)internal;
	const u8* cur_pos = pos;
	pos += el_size;
	return cur_pos;
}


static LibError dump_array(const u8* p,
	size_t el_count, DWORD el_type_id, size_t el_size, DumpState state)
{
	const u8* iterator_internal_pos = p;
	return dump_sequence(array_iterator, &iterator_internal_pos,
		el_count, el_type_id, el_size, state);
}


static const STACKFRAME64* current_stackframe64;

static LibError determine_symbol_address(DWORD id, DWORD UNUSED(type_id), const u8** pp)
{
	const STACKFRAME64* sf = current_stackframe64;

	DWORD data_kind;
	if(!SymGetTypeInfo(hProcess, mod_base, id, TI_GET_DATAKIND, &data_kind))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	switch(data_kind)
	{
	// SymFromIndex will fail
	case DataIsMember:
		// pp is already correct (udt_dump_normal retrieved the offset;
		// we do it that way so we can check it against the total
		// UDT size for safety).
		return ERR_OK;

	// this symbol is defined as static in another module =>
	// there's nothing we can do.
	case DataIsStaticMember:
		return ERR_SYM_UNRETRIEVABLE_STATIC;	// NOWARN

	// ok; will handle below
	case DataIsLocal:
	case DataIsStaticLocal:
	case DataIsParam:
	case DataIsObjectPtr:
	case DataIsFileStatic:
	case DataIsGlobal:
		break;

	default:
		debug_warn("unexpected data_kind");

	//case DataIsConstant

	}

	// get SYMBOL_INFO (we need .Flags)
	SYMBOL_INFO_PACKAGEW2 sp;
	SYMBOL_INFOW* sym = &sp.si;
	if(!SymFromIndexW(hProcess, mod_base, id, sym))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);

DWORD addrofs = 0;
ULONG64 addr2 = 0;
DWORD ofs2 = 0;
SymGetTypeInfo(hProcess, mod_base, id, TI_GET_ADDRESSOFFSET, &addrofs);
SymGetTypeInfo(hProcess, mod_base, id, TI_GET_ADDRESS, &addr2);
SymGetTypeInfo(hProcess, mod_base, id, TI_GET_OFFSET, &ofs2);



	// get address
	ULONG64 addr = sym->Address;
	// .. relative to a register
	//    note: we only have the FP (not SP)
	if(sym->Flags & SYMFLAG_REGREL)
	{
		if(sym->Register == CV_REG_EBP)
			goto fp_rel;
		else
			goto in_register;
	}
	// .. relative to FP (appears to be obsolete)
	else if(sym->Flags & SYMFLAG_FRAMEREL)
	{
fp_rel:
		addr += sf->AddrFrame.Offset;

		// HACK: reg-relative symbols (params and locals, but not
		// static) appear to be off by 4 bytes in release builds.
		// no idea as to the cause, but this "fixes" it.
#ifdef NDEBUG
		addr += sizeof(void*);
#endif
	}
	// .. in register (this happens when optimization is enabled,
	//    but we can't do anything; see SymbolInfoRegister)
	else if(sym->Flags & SYMFLAG_REGISTER)
	{
in_register:
		*pp = (const u8*)(uintptr_t)sym->Register;
		return ERR_SYM_UNRETRIEVABLE_REG;	// NOWARN
	}

	*pp = (const u8*)addr;

debug_printf("SYM| %ws at %p  flags=%X dk=%d sym->addr=%I64X addrofs=%X addr2=%I64X ofs2=%X\n", sym->Name, *pp, sym->Flags, data_kind, sym->Address, addrofs, addr2, ofs2);

	return ERR_OK;
}


//-----------------------------------------------------------------------------
// dump routines for each dbghelp symbol type
//-----------------------------------------------------------------------------

// these functions return != 0 if they're not able to produce any
// reasonable output at all; the caller (dump_sym_data, dump_sequence, etc.)
// will display the appropriate error message via dump_error.
// called by dump_sym; lock is held.

static LibError dump_sym_array(DWORD type_id, const u8* p, DumpState state)
{ 
	ULONG64 size_ = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	// get element count and size
	DWORD el_type_id = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &el_type_id))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	// .. workaround: TI_GET_COUNT returns total struct size for
	//    arrays-of-struct. therefore, calculate as size / el_size.
	ULONG64 el_size_;
	if(!SymGetTypeInfo(hProcess, mod_base, el_type_id, TI_GET_LENGTH, &el_size_))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	const size_t el_size = (size_t)el_size_;
	debug_assert(el_size != 0);
	const size_t num_elements = size/el_size;
	debug_assert(num_elements != 0);
 
	return dump_array(p, num_elements, el_type_id, el_size, state);
}


//-----------------------------------------------------------------------------

static LibError dump_sym_base_type(DWORD type_id, const u8* p, DumpState state)
{
	DWORD base_type;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_BASETYPE, &base_type))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	ULONG64 size_ = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	// single out() call. note: we pass a single u64 for all sizes,
	// which will only work on little-endian systems.
	// must be declared before goto to avoid W4 warning.
	const wchar_t* fmt = L"";

	u64 data = movzx_64le(p, size);
	// if value is 0xCC..CC (uninitialized mem), we display as hex. 
	// the output would otherwise be garbage; this makes it obvious.
	// note: be very careful to correctly handle size=0 (e.g. void*).
	for(size_t i = 0; i < size; i++)
	{
		if(p[i] != 0xCC)
			break;
		if(i == size-1)
			goto display_as_hex;
	}

	switch(base_type)
	{
		// boolean
		case btBool:
			debug_assert(size == sizeof(bool));
			fmt = L"%hs";
			data = (u64)(data? "true " : "false");
			break;

		// floating-point
		case btFloat:
			// C calling convention casts float params to doubles, so printf
			// expects one when we indicate %g. there are no width flags,
			// so we have to manually convert the float data to double.
			if(size == sizeof(float))
				*(double*)&data = (double)*(float*)&data;
			else if(size != sizeof(double))
				debug_warn("dump_sym_base_type: invalid float size");
			fmt = L"%g";
			break;

		// signed integers (displayed as decimal)
		case btInt:
		case btLong:
			if(size != 1 && size != 2 && size != 4 && size != 8)
				debug_warn("dump_sym_base_type: invalid int size");
			// need to re-load and sign-extend, because we output 64 bits.
			data = movsx_64le(p, size);
			fmt = L"%I64d";
			break;

		// unsigned integers (displayed as hex)
		// note: 0x00000000 can get annoying (0 would be nicer),
		// but it indicates the variable size and makes for consistently
		// formatted structs/arrays. (0x1234 0 0x5678 is ugly)
		case btUInt:
		case btULong:
display_as_hex:
			if(size == 1)
			{
				// _TUCHAR
				if(state.indirection)
				{
					state.indirection = 0;
					return dump_array(p, 8, type_id, size, state);
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
			debug_assert(size == sizeof(char) || size == sizeof(wchar_t));
			// char*, wchar_t*
			if(state.indirection)
			{
				state.indirection = 0;
				return dump_array(p, 8, type_id, size, state);
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
			return ERR_SYM_UNSUPPORTED;	// NOWARN
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

	return ERR_OK;
}


//-----------------------------------------------------------------------------

static LibError dump_sym_base_class(DWORD type_id, const u8* p, DumpState state)
{
	DWORD base_class_type_id;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &base_class_type_id))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);

	// this is a virtual base class. we can't display those because it'd
	// require reading the VTbl, which is difficult given lack of documentation
	// and just not worth it.
	DWORD vptr_ofs;
	if(SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_VIRTUALBASEPOINTEROFFSET, &vptr_ofs))
		return ERR_SYM_UNSUPPORTED;	// NOWARN

	return dump_sym(base_class_type_id, p, state);
	

}


//-----------------------------------------------------------------------------

static LibError dump_sym_data(DWORD id, const u8* p, DumpState state)
{
	// display name (of variable/member)
	const wchar_t* name;
	if(!SymGetTypeInfo(hProcess, mod_base, id, TI_GET_SYMNAME, &name))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	out(L"%s = ", name);
	LocalFree((HLOCAL)name);

	__try
	{
		// get type_id and address
		DWORD type_id;
		if(!SymGetTypeInfo(hProcess, mod_base, id, TI_GET_TYPEID, &type_id))
			WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
		LibError ret = determine_symbol_address(id, type_id, &p);
		if(ret != 0)
			return ret;

		// display value recursively
		return dump_sym(type_id, p, state);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return ERR_SYM_INTERNAL_ERROR;	// NOWARN
	}
}


//-----------------------------------------------------------------------------

static LibError dump_sym_enum(DWORD type_id, const u8* p, DumpState UNUSED(state))
{
	ULONG64 size_ = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	const i64 enum_value = movsx_64le(p, size);

	// get array of child symbols (enumerants).
	DWORD num_children;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_CHILDRENCOUNT, &num_children))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	TI_FINDCHILDREN_PARAMS2 fcp(num_children);
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_FINDCHILDREN, &fcp))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	num_children = fcp.p.Count;	// was truncated to MAX_CHILDREN
	const DWORD* children = fcp.p.ChildId;

	// for each child (enumerant):
	for(uint i = 0; i < num_children; i++)
	{
		DWORD child_data_id = children[i];

		// get this enumerant's value. we can't make any assumptions about
		// the variant's type or size  - no restriction is documented.
		// rationale: VariantChangeType is much less tedious than doing
		// it manually and guarantees we cover everything. the OLE DLL is
		// already pulled in by e.g. OpenGL anyway.
		VARIANT v;
		if(!SymGetTypeInfo(hProcess, mod_base, child_data_id, TI_GET_VALUE, &v))
			WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
		if(VariantChangeType(&v, &v, 0, VT_I8) != S_OK)
			continue;

		// it's the one we want - output its name.
		if(enum_value == v.llVal)
		{
			const wchar_t* name;
			if(!SymGetTypeInfo(hProcess, mod_base, child_data_id, TI_GET_SYMNAME, &name))
				WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
			out(L"%s", name);
			LocalFree((HLOCAL)name);
			return ERR_OK;
		}
	}

	// we weren't able to retrieve a matching enum value, but can still
	// produce reasonable output (the numeric value).
	// note: could goto here after a SGTI fails, but we fail instead
	// to make sure those errors are noticed.
	out(L"%I64d", enum_value);
	return ERR_OK;
}


//-----------------------------------------------------------------------------

static LibError dump_sym_function(DWORD UNUSED(type_id), const u8* UNUSED(p),
	DumpState UNUSED(state))
{
	return ERR_SYM_SUPPRESS_OUTPUT;	// NOWARN
}


//-----------------------------------------------------------------------------

static LibError dump_sym_function_type(DWORD UNUSED(type_id), const u8* p, DumpState UNUSED(state))
{
	// this symbol gives class parent, return type, and parameter count.
	// unfortunately the one thing we care about, its name,
	// isn't exposed via TI_GET_SYMNAME, so we resolve it ourselves.

	unlock();	// prevent recursive lock

	char name[DBG_SYMBOL_LEN];
	LibError err = debug_resolve_symbol((void*)p, name, 0, 0);

	lock();

	out(L"0x%p", p);
	if(err == ERR_OK)
		out(L" (%hs)", name);
	return ERR_OK;
}


//-----------------------------------------------------------------------------

// do not follow pointers that we have already displayed. this reduces
// clutter a bit and prevents infinite recursion for cyclical references
// (e.g. via struct S { S* p; } s; s.p = &s;)

typedef std::set<const u8*> PtrSet;
static PtrSet* already_visited_ptrs;
	// allocated on-demand by ptr_already_visited. this cannot be a NLSO
	// because it may be used before _cinit.
	// if we put it in a function, construction still fails on VC7 because
	// the atexit table will not have been initialized yet.

// called by debug_dump_stack and wdbg_sym_shutdown
static void ptr_reset_visited()
{
	delete already_visited_ptrs;
	already_visited_ptrs = 0;
}

static bool ptr_already_visited(const u8* p)
{
	if(!already_visited_ptrs)
		already_visited_ptrs = new PtrSet;

	std::pair<PtrSet::iterator, bool> ret = already_visited_ptrs->insert(p);
	return !ret.second;
}


static LibError dump_sym_pointer(DWORD type_id, const u8* p, DumpState state)
{
	ULONG64 size_ = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	// read+output pointer's value.
	p = (const u8*)movzx_64le(p, size);
	out(L"0x%p", p);

	// bail if it's obvious the pointer is bogus
	// (=> can't display what it's pointing to)
	if(debug_is_pointer_bogus(p))
		return ERR_OK;

	// avoid duplicates and circular references
	if(ptr_already_visited(p))
	{
		out(L" (see above)");
		return ERR_OK;
	}

	// display what the pointer is pointing to.
	// if the pointer is invalid (despite "bogus" check above),
	// dump_data_sym recovers via SEH and prints an error message.
	// if the pointed-to value turns out to uninteresting (e.g. void*),
	// the responsible dump_sym* will erase "->", leaving only address.
	out(L" -> ");
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &type_id))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);

	// prevent infinite recursion just to be safe (shouldn't happen)
	if(state.indirection >= MAX_INDIRECTION)
		WARN_RETURN(ERR_SYM_NESTING_LIMIT);
	state.indirection++;
	return dump_sym(type_id, p, state);
}


//-----------------------------------------------------------------------------


static LibError dump_sym_typedef(DWORD type_id, const u8* p, DumpState state)
{
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &type_id))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	return dump_sym(type_id, p, state);
}


//-----------------------------------------------------------------------------


// determine type and size of the given child in a UDT.
// useful for UDTs that contain typedefs describing their contents,
// e.g. value_type in STL containers.
static LibError udt_get_child_type(const wchar_t* child_name,
	ULONG num_children, const DWORD* children,
	DWORD* el_type_id, size_t* el_size)
{
	*el_type_id = 0;
	*el_size = 0;

	for(ULONG i = 0; i < num_children; i++)
	{
		DWORD child_id = children[i];

		// find the desired child
		wchar_t* this_child_name;
		if(!SymGetTypeInfo(hProcess, mod_base, child_id, TI_GET_SYMNAME, &this_child_name))
			continue;
		const bool found_it = !wcscmp(this_child_name, child_name);
		LocalFree(this_child_name);
		if(!found_it)
			continue;

		// .. its type information is what we want.
		DWORD type_id;
		if(!SymGetTypeInfo(hProcess, mod_base, child_id, TI_GET_TYPEID, &type_id))
			WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);

		ULONG64 size;
		if(!SymGetTypeInfo(hProcess, mod_base, child_id, TI_GET_LENGTH, &size))
			WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);

		*el_type_id = type_id;
		*el_size = (size_t)size;
		return ERR_OK;
	}

	// (happens if called for containers that are treated as STL but are not)
	return ERR_SYM_CHILD_NOT_FOUND;	// NOWARN
}


static LibError udt_dump_std(const wchar_t* wtype_name, const u8* p, size_t size, DumpState state,
	ULONG num_children, const DWORD* children)
{
	LibError err;

	// not a C++ standard library object; can't handle it.
	if(wcsncmp(wtype_name, L"std::", 5) != 0)
		return INFO_CANNOT_HANDLE;

	// check for C++ objects that should be displayed via udt_dump_normal.
	// STL containers are special-cased and the rest (apart from those here)
	// are ignored, because for the most part they are spew.
	if(!wcsncmp(wtype_name, L"std::pair", 9))
		return INFO_CANNOT_HANDLE;

	// convert to char since debug_stl doesn't support wchar_t.
	char ctype_name[DBG_SYMBOL_LEN];
	snprintf(ctype_name, ARRAY_SIZE(ctype_name), "%ws", wtype_name);

	// display contents of STL containers
	// .. get element type
	DWORD el_type_id;
	size_t el_size;
 	err = udt_get_child_type(L"value_type", num_children, children, &el_type_id, &el_size);
	if(err != ERR_OK)
		goto not_valid_container;
	// .. get iterator and # elements
	size_t el_count;
	DebugIterator el_iterator;
	u8 it_mem[DEBUG_STL_MAX_ITERATOR_SIZE];
	err = stl_get_container_info(ctype_name, p, size, el_size, &el_count, &el_iterator, it_mem);
	if(err != ERR_OK)
		goto not_valid_container;
	return dump_sequence(el_iterator, it_mem, el_count, el_type_id, el_size, state);
not_valid_container:

	// build and display detailed "error" message.
	char buf[100];
	const char* text;
	// .. object named std::* but doesn't include a "value_type" child =>
	//    it's a non-STL C++ stdlib object. wasn't handled by the
	//    special case above, so we just display its simplified type name
	//    (the contents are usually spew).
	if(err == ERR_SYM_CHILD_NOT_FOUND)
		text = "";
	// .. not one of the containers we can analyse.
	if(err == ERR_STL_CNT_UNKNOWN)
		text = "unsupported ";
	// .. container of a known type but contents are invalid.
	if(err == ERR_STL_CNT_INVALID)
		text = "uninitialized/invalid ";
	// .. some other error encountered
	else
	{
		snprintf(buf, ARRAY_SIZE(buf), "error %d while analyzing ", err);
		text = buf;
	}
	out(L"(%hs%hs)", text, stl_simplify_name(ctype_name));
	return ERR_OK;
}


static bool udt_should_suppress(const wchar_t* type_name)
{
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


static LibError udt_dump_suppressed(const wchar_t* type_name, const u8* UNUSED(p), size_t UNUSED(size),
	DumpState state, ULONG UNUSED(num_children), const DWORD* UNUSED(children))
{
	if(!udt_should_suppress(type_name))
		return INFO_CANNOT_HANDLE;

	// the data symbol is pointer-to-UDT. since we won't display its
	// contents, leave only the pointer's value.
	if(state.indirection)
		out_erase(4);	// " -> "

	// indicate something was deliberately left out
	// (otherwise, lack of output may be taken for an error)
	out(L" (..)");

	return ERR_OK;
}


// (by now) non-trivial heuristic to determine if a UDT should be
// displayed on one line or several. split out of udt_dump_normal.
static bool udt_fits_on_one_line(const wchar_t* type_name, size_t child_count, size_t total_size)
{
	// special case: always put CStr* on one line
	// (std::*string are displayed directly, but these go through
	// udt_dump_normal. we want to avoid the ensuing 3-line output)
	if(!wcscmp(type_name, L"CStr") || !wcscmp(type_name, L"CStr8") || !wcscmp(type_name, L"CStrW"))
		return true;

	// try to get actual number of relevant children
	// (typedefs etc. are never displayed, but are included in child_count.
	// we have to balance that vs. tons of static members, which aren't
	// reflected in total_size).
	// .. prevent division by 0.
	if(child_count == 0)
		child_count = 1;
	// special-case a few types that would otherwise be classified incorrectly
	// (due to having more or less than expected relevant children)
	if(!wcsncmp(type_name, L"std::pair", 9))
		child_count = 2;

	const size_t avg_size = total_size / child_count;
		// (if 0, no worries - child_count will probably be large and
		// we return false, which is a safe default)

	// small UDT with a few (small) members: fits on one line.
	if(child_count <= 3 && avg_size <= sizeof(int))
		return true;

	return false;
}


static LibError udt_dump_normal(const wchar_t* type_name, const u8* p, size_t size,
	DumpState state, ULONG num_children, const DWORD* children)
{
	const bool fits_on_one_line = udt_fits_on_one_line(type_name, num_children, size);

	// prevent infinite recursion just to be safe (shouldn't happen)
	if(state.level >= MAX_LEVEL)
		WARN_RETURN(ERR_SYM_NESTING_LIMIT);
	state.level++;

	out(fits_on_one_line? L"{ " : L"\r\n");

	bool displayed_anything = false;
	for(ULONG i = 0; i < num_children; i++)
	{
		const DWORD child_id = children[i];

		// get offset. if not available, skip this child
		// (we only display data here, not e.g. typedefs)
		DWORD ofs = 0;
		if(!SymGetTypeInfo(hProcess, mod_base, child_id, TI_GET_OFFSET, &ofs))
			continue;
		debug_assert(ofs < size);

		if(!fits_on_one_line)
			INDENT;

		const u8* el_p = p+ofs;
		LibError err = dump_sym(child_id, el_p, state);

		// there was no output for this child; undo its indentation (if any),
		// skip everything below and proceed with the next child.
		if(err == ERR_SYM_SUPPRESS_OUTPUT)
		{
			if(!fits_on_one_line)
				UNINDENT;
			continue;
		}

		displayed_anything = true;
		dump_error(err, el_p);	// nop if err == ERR_OK
		out(fits_on_one_line? L", " : L"\r\n");

		if(err == ERR_SYM_SINGLE_SYMBOL_LIMIT)
			break;
	}	// for each child

	state.level--;

	if(!displayed_anything)
	{
		out_erase(2);	// "{ " or "\r\n"
		out(L"(%s)", type_name);
		return ERR_OK;
	}

	// remove trailing comma separator
	// note: we can't avoid writing it by checking if i == num_children-1:
	// each child might be the last valid data member.
	if(fits_on_one_line)
	{
		out_erase(2);	// ", "
		out(L" }");
	}

	return ERR_OK;
}


static LibError dump_sym_udt(DWORD type_id, const u8* p, DumpState state)
{
	ULONG64 size_ = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	// get array of child symbols (members/functions/base classes).
	DWORD num_children;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_CHILDRENCOUNT, &num_children))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	TI_FINDCHILDREN_PARAMS2 fcp(num_children);
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_FINDCHILDREN, &fcp))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	num_children = fcp.p.Count;	// was truncated to MAX_CHILDREN
	const DWORD* children = fcp.p.ChildId;

	const wchar_t* type_name;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMNAME, &type_name))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);

	LibError ret;
	// note: order is important (e.g. STL special-case must come before
	// suppressing UDTs, which tosses out most other C++ stdlib classes)

	ret = udt_dump_std       (type_name, p, size, state, num_children, children);
	if(ret != INFO_CANNOT_HANDLE)
		goto done;

	ret = udt_dump_suppressed(type_name, p, size, state, num_children, children);
	if(ret != INFO_CANNOT_HANDLE)
		goto done;

	ret = udt_dump_normal    (type_name, p, size, state, num_children, children);
	if(ret != INFO_CANNOT_HANDLE)
		goto done;

done:
	LocalFree((HLOCAL)type_name);
	return ret;
}


//-----------------------------------------------------------------------------


static LibError dump_sym_vtable(DWORD UNUSED(type_id), const u8* UNUSED(p), DumpState UNUSED(state))
{
	// unsupported (vtable internals are undocumented; too much work).
	return ERR_SYM_SUPPRESS_OUTPUT;	// NOWARN
}


//-----------------------------------------------------------------------------


static LibError dump_sym_unknown(DWORD type_id, const u8* UNUSED(p), DumpState UNUSED(state))
{
	// redundant (already done in dump_sym), but this is rare.
	DWORD type_tag;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMTAG, &type_tag))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);

	debug_printf("SYM| unknown tag: %d\n", type_tag);
	out(L"(unknown symbol type)");
	return ERR_OK;
}


//-----------------------------------------------------------------------------


// write name and value of the symbol <type_id> to the output buffer.
// delegates to dump_sym_* depending on the symbol's tag.
static LibError dump_sym(DWORD type_id, const u8* p, DumpState state)
{
	RETURN_ERR(out_check_limit());

	DWORD type_tag;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMTAG, &type_tag))
		WARN_RETURN(ERR_SYM_TYPE_INFO_UNAVAILABLE);
	switch(type_tag)
	{
	case SymTagArrayType:
		return dump_sym_array         (type_id, p, state);
	case SymTagBaseType:
		return dump_sym_base_type     (type_id, p, state);
	case SymTagBaseClass:
		return dump_sym_base_class    (type_id, p, state);
	case SymTagData:
		return dump_sym_data          (type_id, p, state);
	case SymTagEnum:
		return dump_sym_enum          (type_id, p, state);
	case SymTagFunction:
		return dump_sym_function      (type_id, p, state);
	case SymTagFunctionType:
		return dump_sym_function_type (type_id, p, state);
	case SymTagPointerType:
		return dump_sym_pointer       (type_id, p, state);
	case SymTagTypedef:
		return dump_sym_typedef       (type_id, p, state);
	case SymTagUDT:
		return dump_sym_udt           (type_id, p, state);
	case SymTagVTable:
		return dump_sym_vtable        (type_id, p, state);
	default:
		return dump_sym_unknown       (type_id, p, state);
	}
}


//////////////////////////////////////////////////////////////////////////////
//
// stack trace
//
//////////////////////////////////////////////////////////////////////////////

// output the symbol's name and value via dump_sym*.
// called from dump_frame_cb for each local symbol; lock is held.
static BOOL CALLBACK dump_sym_cb(SYMBOL_INFO* sym, ULONG UNUSED(size), void* UNUSED(ctx))
{
	out_latch_pos();	// see decl
	mod_base = sym->ModBase;
	const u8* p = (const u8*)sym->Address;
	DumpState state;

	INDENT;
	LibError err = dump_sym(sym->Index, p, state);
	dump_error(err, p);
	if(err == ERR_SYM_SUPPRESS_OUTPUT)
		UNINDENT;
	else
		out(L"\r\n");

	return TRUE;	// continue
}


//////////////////////////////////////////////////////////////////////////////


struct IMAGEHLP_STACK_FRAME2 : public IMAGEHLP_STACK_FRAME
{
	IMAGEHLP_STACK_FRAME2(const STACKFRAME64* sf)
	{
		// apparently only PC, FP and SP are necessary, but
		// we go whole-hog to be safe.
		memset(this, 0, sizeof(IMAGEHLP_STACK_FRAME2));
		InstructionOffset  = sf->AddrPC.Offset;
		ReturnOffset       = sf->AddrReturn.Offset;
		FrameOffset        = sf->AddrFrame.Offset;
		StackOffset        = sf->AddrStack.Offset;
		BackingStoreOffset = sf->AddrBStore.Offset;
		FuncTableEntry     = (ULONG64)sf->FuncTableEntry;
		Virtual            = sf->Virtual;
		// (note: array of different types, can't copy directly)
		for(int i = 0; i < 4; i++)
			Params[i] = sf->Params[i];
	}
};

// called by walk_stack for each stack frame
static LibError dump_frame_cb(const STACKFRAME64* sf, void* UNUSED(user_arg))
{
	current_stackframe64 = sf;
	void* func = (void*)sf->AddrPC.Offset;

	char func_name[DBG_SYMBOL_LEN]; char file[DBG_FILE_LEN]; int line;
	if(debug_resolve_symbol(func, func_name, file, &line) == 0)
	{
		// don't trace back further than the app's entry point
		// (noone wants to see this frame). checking for the
		// function name isn't future-proof, but not stopping is no big deal.
		// an alternative would be to check if module=kernel32, but
		// that would cut off callbacks as well.
		if(!strcmp(func_name, "_BaseProcessStart@4"))
			return ERR_OK;

		out(L"%hs (%hs:%d)\r\n", func_name, file, line);
	}
	else
		out(L"%p\r\n", func);

	// only enumerate symbols for this stack frame
	// (i.e. its locals and parameters)
	// problem: debug info is scope-aware, so we won't see any variables
	// declared in sub-blocks. we'd have to pass an address in that block,
	// which isn't worth the trouble. since 
	IMAGEHLP_STACK_FRAME2 imghlp_frame(sf);
	SymSetContext(hProcess, &imghlp_frame, 0);	// last param is ignored

	SymEnumSymbols(hProcess, 0, 0, dump_sym_cb, 0);
		// 2nd and 3rd params indicate scope set by SymSetContext
		// should be used.

	out(L"\r\n");
	return INFO_CB_CONTINUE;
}


// write a complete stack trace (including values of local variables) into
// the specified buffer. if <context> is nonzero, it is assumed to be a
// platform-specific representation of execution state (e.g. Win32 CONTEXT)
// and tracing starts there; this is useful for exceptions.
// otherwise, tracing starts at the current stack position, and the given
// number of stack frames (i.e. functions) above the caller are skipped.
// this prevents functions like debug_assert_failed from
// cluttering up the trace. returns the buffer for convenience.
const wchar_t* debug_dump_stack(wchar_t* buf, size_t max_chars, uint skip, void* pcontext)
{
	static uintptr_t already_in_progress;
	if(!CAS(&already_in_progress, 0, 1))
	{
		wcscpy_s(buf, max_chars,
			L"(cannot start a nested stack trace; what probably happened is that "
			L"an debug_assert/debug_warn/CHECK_ERR fired during the current trace.)"
		);
		return buf;
	}

	if(!pcontext)
		skip++;	// skip this frame

	lock();

	out_init(buf, max_chars);
	ptr_reset_visited();

	LibError err = walk_stack(dump_frame_cb, 0, skip, (const CONTEXT*)pcontext);
	if(err != ERR_OK)
		out(L"(error while building stack trace: %d)", err);

	unlock();

	already_in_progress = 0;
	return buf;
}





// write out a "minidump" containing register and stack state; this enables
// examining the crash in a debugger. called by wdbg_exception_filter.
// heavily modified from http://www.codeproject.com/debug/XCrashReportPt3.asp
// lock must be held.
void wdbg_write_minidump(EXCEPTION_POINTERS* exception_pointers)
{
	lock();

	// note: we go through some gyrations here (strcpy+strcat) to avoid
	// dependency on file code (vfs_path_append).
	char N_path[PATH_MAX];
	strcpy_s(N_path, ARRAY_SIZE(N_path), ah_get_log_dir());
	strcat_s(N_path, ARRAY_SIZE(N_path), "crashlog.dmp");
	HANDLE hFile = CreateFile(N_path, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
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
		DISPLAY_ERROR(L"Unable to generate minidump.");
	}

	CloseHandle(hFile);
	unlock();
}




static LibError wdbg_sym_shutdown()
{
	ptr_reset_visited();
	return sym_shutdown();
}


//----------------------------------------------------------------------------
// built-in self test
//----------------------------------------------------------------------------

#if SELF_TEST_ENABLED
namespace test {
#pragma optimize("", off)



static void test_array()
{
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

	Large large_array_of_large_structs[8] = { { 0.0,0.0,0.0,0.0 } }; UNUSED2(large_array_of_large_structs);
	Large small_array_of_large_structs[2] = { { 0.0,0.0,0.0,0.0 } }; UNUSED2(small_array_of_large_structs);
	Small large_array_of_small_structs[8] = { { 1,2 } }; UNUSED2(large_array_of_small_structs);
	Small small_array_of_small_structs[2] = { { 1,2 } }; UNUSED2(small_array_of_small_structs);

	int ints[] = { 1,2,3,4,5 };	UNUSED2(ints);
	wchar_t chars[] = { 'w','c','h','a','r','s',0 }; UNUSED2(chars);

	// note: prefer simple error (which also generates stack trace) to
	// exception, because it is guaranteed to work (no issues with the
	// debugger swallowing exceptions).
	DISPLAY_ERROR(L"wdbg_sym self test: check if stack trace below is ok.");
	//RaiseException(0xf001,0,0,0);
}

// also used by test_stl as an element type
struct Nested
{
	int nested_member;
	struct Nested* self_ptr;
};

static void test_udt()
{
	Nested nested = { 123 }; nested.self_ptr = &nested;

	typedef struct
	{
		u8 s1;
		u8 s2;
		char s3;
	}
	Small;
	Small small__ = { 0x55, 0xaa, -1 }; UNUSED2(small__);

	struct Large
	{
		u8 large_member_u8;
		std::string large_member_string;
		double large_member_double;
	}
	large = { 0xff, "large struct string", 123456.0 }; UNUSED2(large);


	class Base
	{
		int base_int;
		std::wstring base_wstring;
	public:
		Base()
			: base_int(123), base_wstring(L"base wstring")
		{
		}
	};
	class Derived : private Base
	{
		double derived_double;
	public:
		Derived()
			: derived_double(-1.0)
		{
		}
	}
	derived;

	test_array();
}

// STL containers and their contents
static void test_stl()
{
	std::vector<std::wstring> v_wstring;
	v_wstring.push_back(L"ws1"); v_wstring.push_back(L"ws2");

	std::deque<int> d_int;
	d_int.push_back(1); d_int.push_back(2); d_int.push_back(3);
	std::deque<std::string> d_string;
	d_string.push_back("a"); d_string.push_back("b"); d_string.push_back("c");

	std::list<float> l_float;
	l_float.push_back(0.1f); l_float.push_back(0.2f); l_float.push_back(0.3f); l_float.push_back(0.4f); 

	std::map<std::string, int> m_string_int;
	m_string_int.insert(std::make_pair<std::string,int>("s5", 5));
	m_string_int.insert(std::make_pair<std::string,int>("s6", 6));
	m_string_int.insert(std::make_pair<std::string,int>("s7", 7));
	std::map<int, std::string> m_int_string;
	m_int_string.insert(std::make_pair<int,std::string>(1, "s1"));
	m_int_string.insert(std::make_pair<int,std::string>(2, "s2"));
	m_int_string.insert(std::make_pair<int,std::string>(3, "s3"));
	std::map<int, int> m_int_int;
	m_int_int.insert(std::make_pair<int,int>(1, 1));
	m_int_int.insert(std::make_pair<int,int>(2, 2));
	m_int_int.insert(std::make_pair<int,int>(3, 3));

	STL_HASH_MAP<std::string, int> hm_string_int;
	hm_string_int.insert(std::make_pair<std::string,int>("s5", 5));
	hm_string_int.insert(std::make_pair<std::string,int>("s6", 6));
	hm_string_int.insert(std::make_pair<std::string,int>("s7", 7));
	STL_HASH_MAP<int, std::string> hm_int_string;
	hm_int_string.insert(std::make_pair<int,std::string>(1, "s1"));
	hm_int_string.insert(std::make_pair<int,std::string>(2, "s2"));
	hm_int_string.insert(std::make_pair<int,std::string>(3, "s3"));
	STL_HASH_MAP<int, int> hm_int_int;
	hm_int_int.insert(std::make_pair<int,int>(1, 1));
	hm_int_int.insert(std::make_pair<int,int>(2, 2));
	hm_int_int.insert(std::make_pair<int,int>(3, 3));


	std::set<uintptr_t> s_uintptr;
	s_uintptr.insert(0x123); s_uintptr.insert(0x456);

	// empty
	std::deque<u8> d_u8_empty;
	std::list<Nested> l_nested_empty;
	std::map<double,double> m_double_empty;
	std::multimap<int,u8> mm_int_empty;
	std::set<uint> s_uint_empty;
	std::multiset<char> ms_char_empty;
	std::vector<double> v_double_empty;
	std::queue<double> q_double_empty;
	std::stack<double> st_double_empty;
#if HAVE_STL_HASH
	STL_HASH_MAP<double,double> hm_double_empty;
	STL_HASH_MULTIMAP<double,std::wstring> hmm_double_empty;
	STL_HASH_SET<double> hs_double_empty;
	STL_HASH_MULTISET<double> hms_double_empty;
#endif
#if HAVE_STL_SLIST
	STL_SLIST<double> sl_double_empty;
#endif
	std::string str_empty;
	std::wstring wstr_empty;

	test_udt();

	// uninitialized
	std::deque<u8> d_u8_uninit;
	std::list<Nested> l_nested_uninit;
	std::map<double,double> m_double_uninit;
	std::multimap<int,u8> mm_int_uninit;
	std::set<uint> s_uint_uninit;
	std::multiset<char> ms_char_uninit;
	std::vector<double> v_double_uninit;
	std::queue<double> q_double_uninit;
	std::stack<double> st_double_uninit;
#if HAVE_STL_HASH
	STL_HASH_MAP<double,double> hm_double_uninit;
	STL_HASH_MULTIMAP<double,std::wstring> hmm_double_uninit;
	STL_HASH_SET<double> hs_double_uninit;
	STL_HASH_MULTISET<double> hms_double_uninit;
#endif
#if HAVE_STL_SLIST
	STL_SLIST<double> sl_double_uninit;
#endif
	std::string str_uninit;
	std::wstring wstr_uninit;
}


// also exercises all basic types because we need to display some values
// anyway (to see at a glance whether symbol engine addrs are correct)
static void test_addrs(int p_int, double p_double, char* p_pchar, uintptr_t p_uintptr)
{
	debug_printf("\nTEST_ADDRS\n");

	uint l_uint = 0x1234;
	bool l_bool = true; UNUSED2(l_bool);
	wchar_t l_wchars[] = L"wchar string";
	enum TestEnum { VAL1=1, VAL2=2 } l_enum = VAL1;
	u8 l_u8s[] = { 1,2,3,4 };
	void (*l_funcptr)(void) = test_stl;

	static double s_double = -2.718;
	static char s_chars[] = {'c','h','a','r','s',0};
	static void (*s_funcptr)(int, double, char*, uintptr_t) = test_addrs;
	static void* s_ptr = (void*)(uintptr_t)0x87654321;
	static HDC s_hdc = (HDC)0xff0;

	debug_printf("p_int     addr=%p val=%d\n", &p_int, p_int);
	debug_printf("p_double  addr=%p val=%g\n", &p_double, p_double);
	debug_printf("p_pchar   addr=%p val=%s\n", &p_pchar, p_pchar);
	debug_printf("p_uintptr addr=%p val=%lu\n", &p_uintptr, p_uintptr);

	debug_printf("l_uint    addr=%p val=%u\n", &l_uint, l_uint);
	debug_printf("l_wchars  addr=%p val=%ws\n", &l_wchars, l_wchars);
	debug_printf("l_enum    addr=%p val=%d\n", &l_enum, l_enum);
	debug_printf("l_u8s     addr=%p val=%d\n", &l_u8s, l_u8s);
	debug_printf("l_funcptr addr=%p val=%p\n", &l_funcptr, l_funcptr);

	test_stl();

	int uninit_int; UNUSED2(uninit_int);
	float uninit_float; UNUSED2(uninit_float);
	double uninit_double; UNUSED2(uninit_double);
	bool uninit_bool; UNUSED2(uninit_bool);
	HWND uninit_hwnd; UNUSED2(uninit_hwnd);
}


static void self_test()
{
	test_addrs(123, 3.1415926535897932384626, "pchar string", 0xf00d);
}

SELF_TEST_RUN;

#pragma optimize("", on)
}	// namespace test
#endif	// #if SELF_TEST_ENABLED
