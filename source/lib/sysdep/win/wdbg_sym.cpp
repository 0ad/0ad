// Win32 stack trace and symbol engine
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
#include "sysdep/cpu.h"
#include "wdbg.h"
#include "debug_stl.h"

// optional: enables translation of the "unhandled exception" dialog.
#ifdef I18N
#include "ps/i18n.h"
#endif

#ifndef PERFORM_SELF_TEST
#define PERFORM_SELF_TEST 0
#endif

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



enum WdbgError
{
	WDBG_NO_STACK_FRAMES_FOUND = -100000,

	// the value is stored in an external module and therefore cannot be
	// displayed.
	WDBG_UNRETRIEVABLE_STATIC  = -100100,

	// the value is stored in a register and therefore cannot be displayed
	// (see CV_HREG_e).
	WDBG_UNRETRIEVABLE_REG     = -100101,

	// an essential call to SymGetTypeInfo or SymFromIndex failed.
	WDBG_TYPE_INFO_UNAVAILABLE = -100102,

	// a generous limit on nesting depth has been reached.
	// we abort to make sure we don't recurse infinitely.
	WDBG_NESTING_LIMIT         = -100103,

	// too much output has been produced by this top-level symbol;
	// we must terminate recursion lest it hog all buffer space.
	// dump will continue with the next top-level symbol.
	WDBG_SINGLE_SYMBOL_LIMIT   = -100104,

	// exception raised while processing the symbol.
	WDBG_INTERNAL_ERROR        = -100200,

	// something required to completely display the symbol isn't implemented.
	WDBG_UNSUPPORTED           = -100201,

	// this STL container has bogus contents (probably uninitialized)
	WDBG_CONTAINER_INVALID     = -100300,

	// one of the dump_sym* functions decided not to output anything at
	// all (e.g. for member functions in UDTs - we don't want those).
	// therefore, skip any post-symbol formatting (e.g. ",") as well.
	WDBG_SUPPRESS_OUTPUT       = -110000
};


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

static const STACKFRAME64* current_stackframe64;

// call on-demand (allows handling exceptions raised before win.cpp
// init functions are called); no effect if already initialized.
static int sym_init()
{
	// bail if already initialized (there's nothing to do).
	static uintptr_t already_initialized = 0;
	if(!CAS(&already_initialized, 0, 1))
		return 0;

	hProcess = GetCurrentProcess();

	SymSetOptions(SYMOPT_DEFERRED_LOADS/*/*|SYMOPT_DEBUG*/);
	// loads symbols for all active modules.
	BOOL ok = SymInitialize(hProcess, 0, TRUE);
	debug_assert(ok);

	mod_base = SymGetModuleBase64(hProcess, (u64)&sym_init);
	IMAGE_NT_HEADERS* header = ImageNtHeader((void*)mod_base);
	machine = header->FileHeader.Machine;

	return 0;
}


// called from wdbg_sym_shutdown.
static int sym_shutdown()
{
	SymCleanup(hProcess);
	return 0;
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
int debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line)
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
	return (successes == 0)? -1 : 0;
}


//----------------------------------------------------------------------------
// stack walk via dbghelp
//----------------------------------------------------------------------------

// rationale: to function properly, StackWalk64 requires a CONTEXT on
// non-x86 systems (documented) or when in release mode (observed).
// exception handlers can call walk_stack with their context record;
// otherwise (e.g. dump_stack from debug_assert), we need to query it.
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

#if CPU_IA32

// optimized for size.
// this is the (so far) only case where __declspec(naked) is absolutely
// critical. compiler-generated prolog code trashes EBP and ESP,
// which is especially bad here because the stack trace code relies
// on us returning their correct values.
static __declspec(naked) void get_current_context(void* pcontext)
{
	// squelch W4 unused paramter warning (it's accessed from asm)
	UNUSED2(pcontext);
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
	xchg	eax, ebp			;// ebp restored by POPAD
	stosd
	mov		eax, [esp+4+32]		;// return address
	sub		eax, 5				;// skip CALL instruction -> call site.
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

#else	// #if CPU_IA32

static void get_current_context(CONTEXT* pcontext)
{
	__try
	{
		RaiseException(0xF001, 0, 0, 0);
	}
	__except(*pcontext = (GetExceptionInformation())->ContextRecord, EXCEPTION_CONTINUE_EXECUTION)
	{
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
// return an error if callback never succeeded (returned 0).
// lock must be held.
static int walk_stack(StackFrameCallback cb, void* user_arg = 0, uint skip = 0, const CONTEXT* pcontext = 0)
{
	sym_init();

	const HANDLE hThread = GetCurrentThread();

	// get CONTEXT (see above)
	CONTEXT context;
	// .. caller knows the context (most likely from an exception);
	//    since StackWalk64 may modify it, copy to a local variable.
	if(pcontext)
		context = *pcontext;
	// .. need to determine context ourselves.
	else
	{
		get_current_context(&context);
		skip++;	// skip this frame
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
	int ret = WDBG_NO_STACK_FRAMES_FOUND;
	for(;;)
	{
		BOOL ok = StackWalk64(machine, hProcess, hThread, &sf, (void*)pcontext,
			0, SymFunctionTableAccess64, SymGetModuleBase64, 0);

		// no more frames found - abort.
		// note: also test FP because StackWalk64 sometimes erroneously
		// reports success. unfortunately it doesn't SetLastError either,
		// so we can't indicate the cause of failure. *sigh*
		if(!ok || !sf.AddrFrame.Offset)
			return ret;

		if(skip)
		{
			skip--;
			continue;
		}

		ret = cb(&sf, user_arg);
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
	int err = walk_stack(nth_caller_cb, &func, skip, (const CONTEXT*)pcontext);

	unlock();
	return (err == 0)? func : 0;
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
// return WDBG_SINGLE_SYMBOL_LIMIT once and thereafter WDBG_SUPPRESS_OUTPUT.
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
			debug_warn("out: apparently wrote more than out_chars_left");
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
static int out_check_limit()
{
	if(out_have_warned_of_limit)
		return WDBG_SUPPRESS_OUTPUT;
	if(out_pos - out_latched_pos > 3000)	// ~30 lines
	{
		out_have_warned_of_limit = true;
		return WDBG_SINGLE_SYMBOL_LIMIT;
	}

	// no limit hit, proceed normally
	return 0;
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
static int dump_sym(DWORD id, const u8* p, DumpState state);

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


static void dump_error(int err, const u8* p)
{
	switch(err)
	{
	case 0:
		// no error => no output
		break;
	case WDBG_SINGLE_SYMBOL_LIMIT:
		out(L"(too much output; skipping to next top-level symbol)");
		break;
	case WDBG_UNRETRIEVABLE_STATIC:
		out(L"(unavailable - located in another module)");
		break;
	case WDBG_UNRETRIEVABLE_REG:
		out(L"(unavailable - stored in register %s)", string_for_register((CV_HREG_e)(uintptr_t)p));
		break;
	case WDBG_TYPE_INFO_UNAVAILABLE:
		out(L"(unavailable - type info request failed (GLE=%d))", GetLastError());
		break;
	case WDBG_INTERNAL_ERROR:
		out(L"(unavailable - internal error)\r\n");
		break;
	case WDBG_SUPPRESS_OUTPUT:
		// not an error; do not output anything. handled by caller.
		break;
	default:
		out(L"(unavailable - unspecified error 0x%X (%d))", err, err);
		break;
	}
}


// split out of dump_sequence.
static int dump_string(const u8* p, size_t el_size)
{
	// not char or wchar_t string
	if(el_size != sizeof(char) && el_size != sizeof(wchar_t))
		return 1;
	// not text
	if(!is_string(p, el_size))
		return 1;

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


static int dump_sequence(DebugIterator el_iterator, void* internal,
	size_t el_count, DWORD el_type_id, size_t el_size, DumpState state)
{
	const u8* el_p = 0;	// avoid "uninitialized" warning

	// special case: display as a string if the sequence looks to be text.
	// do this only if container isn't empty because the otherwise the
	// iterator may crash.
	if(el_count)
	{
		el_p = el_iterator(internal, el_size);

		int ret = dump_string(el_p, el_size);
		if(ret == 0)
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

		int err = dump_sym(el_type_id, el_p, state);
		el_p = el_iterator(internal, el_size);

		// there was no output for this child; undo its indentation (if any),
		// skip everything below and proceed with the next child.
		if(err == WDBG_SUPPRESS_OUTPUT)
		{
			if(!fits_on_one_line)
				UNINDENT;
			continue;
		}

		dump_error(err, el_p);	// nop if err == 0
		// add separator unless this is the last element (can't just
		// erase below due to additional "...").
		if(i != num_elements_to_show-1)
			out(fits_on_one_line? L", " : L"\r\n");

		if(err == WDBG_SINGLE_SYMBOL_LIMIT)
			break;
	}	// for each child

	// indicate some elements were skipped
	if(el_count != num_elements_to_show)
		out(L" ...");

	state.level--;
	if(fits_on_one_line)
		out(L" }");
	return 0;
}


static const u8* array_iterator(void* internal, size_t el_size)
{
	const u8*& pos = *(const u8**)internal;
	const u8* cur_pos = pos;
	pos += el_size;
	return cur_pos;
}


static int dump_array(const u8* p,
	size_t el_count, DWORD el_type_id, size_t el_size, DumpState state)
{
	const u8* iterator_internal_pos = p;
	return dump_sequence(array_iterator, &iterator_internal_pos,
		el_count, el_type_id, el_size, state);
}




static int determine_symbol_address(DWORD id, DWORD UNUSED(type_id), const u8** pp)
{
	const STACKFRAME64* sf = current_stackframe64;

	DWORD data_kind;
	if(!SymGetTypeInfo(hProcess, mod_base, id, TI_GET_DATAKIND, &data_kind))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	switch(data_kind)
	{
	// SymFromIndex will fail
	case DataIsMember:
		// pp is already correct (udt_dump_normal retrieved the offset;
		// we do it that way so we can check it against the total
		// UDT size for safety).
		return 0;

	// this symbol is defined as static in another module =>
	// there's nothing we can do.
	case DataIsStaticMember:
		return WDBG_UNRETRIEVABLE_STATIC;

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
		return WDBG_TYPE_INFO_UNAVAILABLE;

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
		return WDBG_UNRETRIEVABLE_REG;
	}

	*pp = (const u8*)addr;

debug_printf("DET_SYM_ADDR %ws at %p  flags=%X dk=%d sym->addr=%I64X addrofs=%X addr2=%I64X ofs2=%X\n", sym->Name, *pp, sym->Flags, data_kind, sym->Address, addrofs, addr2, ofs2);

	return 0;
}


//-----------------------------------------------------------------------------
// dump routines for each dbghelp symbol type
//-----------------------------------------------------------------------------

// these functions return != 0 if they're not able to produce any
// reasonable output at all; the caller (dump_sym_data, dump_sequence, etc.)
// will display the appropriate error message via dump_error.
// called by dump_sym; lock is held.

static int dump_sym_array(DWORD type_id, const u8* p, DumpState state)
{ 
	ULONG64 size_ = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	const size_t size = (size_t)size_;

	// get element count and size
	DWORD el_type_id = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &el_type_id))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	// .. workaround: TI_GET_COUNT returns total struct size for
	//    arrays-of-struct. therefore, calculate as size / el_size.
	ULONG64 el_size_;
	if(!SymGetTypeInfo(hProcess, mod_base, el_type_id, TI_GET_LENGTH, &el_size_))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	const size_t el_size = (size_t)el_size_;
	debug_assert(el_size != 0);
	const size_t num_elements = size/el_size;
	debug_assert(num_elements != 0);
 
	return dump_array(p, num_elements, el_type_id, el_size, state);
}


//-----------------------------------------------------------------------------

static int dump_sym_base_type(DWORD type_id, const u8* p, DumpState state)
{
	DWORD base_type;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_BASETYPE, &base_type))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	ULONG64 size_ = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		return WDBG_TYPE_INFO_UNAVAILABLE;
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
			return WDBG_UNSUPPORTED;
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


//-----------------------------------------------------------------------------

static int dump_sym_base_class(DWORD type_id, const u8* p, DumpState state)
{
	DWORD base_class_type_id;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &base_class_type_id))
		return WDBG_TYPE_INFO_UNAVAILABLE;

	// this is a virtual base class. we can't display those because it'd
	// require reading the VTbl, which is difficult given lack of documentation
	// and just not worth it.
	DWORD vptr_ofs;
	if(SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_VIRTUALBASEPOINTEROFFSET, &vptr_ofs))
		return WDBG_UNSUPPORTED;

	return dump_sym(base_class_type_id, p, state);
	

}


//-----------------------------------------------------------------------------

static int dump_sym_data(DWORD id, const u8* p, DumpState state)
{
	// display name (of variable/member)
	const wchar_t* name;
	if(!SymGetTypeInfo(hProcess, mod_base, id, TI_GET_SYMNAME, &name))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	out(L"%s = ", name);
	LocalFree((HLOCAL)name);

	__try
	{
		// get type_id and address
		DWORD type_id;
		if(!SymGetTypeInfo(hProcess, mod_base, id, TI_GET_TYPEID, &type_id))
			return WDBG_TYPE_INFO_UNAVAILABLE;
		int ret = determine_symbol_address(id, type_id, &p);
		if(ret != 0)
			return ret;

		// display value recursively
		return dump_sym(type_id, p, state);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return WDBG_INTERNAL_ERROR;
	}
}


//-----------------------------------------------------------------------------

static int dump_sym_enum(DWORD type_id, const u8* p, DumpState UNUSED(state))
{
	ULONG64 size_ = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	const size_t size = (size_t)size_;

	const i64 enum_value = movsx_64le(p, size);

	// get array of child symbols (enumerants).
	DWORD num_children;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_CHILDRENCOUNT, &num_children))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	TI_FINDCHILDREN_PARAMS2 fcp(num_children);
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_FINDCHILDREN, &fcp))
		return WDBG_TYPE_INFO_UNAVAILABLE;
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
			return WDBG_TYPE_INFO_UNAVAILABLE;
		if(VariantChangeType(&v, &v, 0, VT_I8) != S_OK)
			continue;

		// it's the one we want - output its name.
		if(enum_value == v.llVal)
		{
			const wchar_t* name;
			if(!SymGetTypeInfo(hProcess, mod_base, child_data_id, TI_GET_SYMNAME, &name))
				return WDBG_TYPE_INFO_UNAVAILABLE;
			out(L"%s", name);
			LocalFree((HLOCAL)name);
			return 0;
		}
	}

	// we weren't able to retrieve a matching enum value, but can still
	// produce reasonable output (the numeric value).
	// note: could goto here after a SGTI fails, but we fail instead
	// to make sure those errors are noticed.
	out(L"%I64d", enum_value);
	return 0;
}


//-----------------------------------------------------------------------------

static int dump_sym_function(DWORD UNUSED(type_id), const u8* UNUSED(p),
	DumpState UNUSED(state))
{
	return WDBG_SUPPRESS_OUTPUT;
}


//-----------------------------------------------------------------------------

static int dump_sym_function_type(DWORD UNUSED(type_id), const u8* p, DumpState UNUSED(state))
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


static int dump_sym_pointer(DWORD type_id, const u8* p, DumpState state)
{
	ULONG64 size_ = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	const size_t size = (size_t)size_;

	// read+output pointer's value.
	p = (const u8*)movzx_64le(p, size);
	out(L"0x%p", p);

	// bail if it's obvious the pointer is bogus
	// (=> can't display what it's pointing to)
	if(debug_is_pointer_bogus(p))
		return 0;

	// avoid duplicates and circular references
	if(ptr_already_visited(p))
	{
		out(L" (see above)");
		return 0;
	}

	// display what the pointer is pointing to.
	// if the pointer is invalid (despite "bogus" check above),
	// dump_data_sym recovers via SEH and prints an error message.
	// if the pointed-to value turns out to uninteresting (e.g. void*),
	// the responsible dump_sym* will erase "->", leaving only address.
	out(L" -> ");
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &type_id))
		return WDBG_TYPE_INFO_UNAVAILABLE;

	// prevent infinite recursion just to be safe (shouldn't happen)
	if(state.indirection >= MAX_INDIRECTION)
		return WDBG_NESTING_LIMIT;
	state.indirection++;
	return dump_sym(type_id, p, state);
}


//-----------------------------------------------------------------------------


static int dump_sym_typedef(DWORD type_id, const u8* p, DumpState state)
{
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &type_id))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	return dump_sym(type_id, p, state);
}


//-----------------------------------------------------------------------------


// determine type and size of the given child in a UDT.
// useful for UDTs that contain typedefs describing their contents,
// e.g. value_type in STL containers.
static int udt_get_child_type(const wchar_t* child_name,
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
			return WDBG_TYPE_INFO_UNAVAILABLE;

		ULONG64 size;
		if(!SymGetTypeInfo(hProcess, mod_base, child_id, TI_GET_LENGTH, &size))
			return WDBG_TYPE_INFO_UNAVAILABLE;

		*el_type_id = type_id;
		*el_size = (size_t)size;
		return 0;
	}

	return 1;
}


static int udt_dump_std(const wchar_t* wtype_name, const u8* p, size_t size, DumpState state,
	ULONG num_children, const DWORD* children)
{
	int err;

	// not a C++ standard library object; can't handle it.
	if(wcsncmp(wtype_name, L"std::", 5) != 0)
		return 1;

	// check for C++ objects that should be displayed via udt_dump_normal.
	// STL containers are special-cased and the rest (apart from those here)
	// are ignored, because for the most part they are spew.
	if(!wcsncmp(wtype_name, L"std::pair", 9))
		return 1;

	// convert to char since debug_stl doesn't support wchar_t.
	char ctype_name[DBG_SYMBOL_LEN];
	snprintf(ctype_name, ARRAY_SIZE(ctype_name), "%ws", wtype_name);

	// display contents of STL containers
	// .. get element type
	DWORD el_type_id;
	size_t el_size;
 	err = udt_get_child_type(L"value_type", num_children, children, &el_type_id, &el_size);
	if(err != 0)
		goto not_valid_container;
	// .. get iterator and # elements
	size_t el_count;
	DebugIterator el_iterator;
	u8 it_mem[DEBUG_STL_MAX_ITERATOR_SIZE];
	err = stl_get_container_info(ctype_name, p, size, el_size, &el_count, &el_iterator, it_mem);
	if(err != 0)
		goto not_valid_container;
	return dump_sequence(el_iterator, it_mem, el_count, el_type_id, el_size, state);
not_valid_container:

	// build and display detailed "error" message.
	char buf[100];
	const char* text = buf;
	// .. C++ stdlib object that we didn't want to display
	//    (just output its simplified type)
	if(err == STL_CNT_UNKNOWN || err == 1)
		text = "";
	// .. container of a known type but contents are invalid
	if(err == STL_CNT_INVALID)
		text = "uninitialized/invalid ";
	// .. some other error encountered
	else
		snprintf(buf, ARRAY_SIZE(buf), "error %d while analyzing ", err);
	out(L"(%hs%hs)", text, stl_simplify_name(ctype_name));
	return 0;
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


static int udt_dump_suppressed(const wchar_t* type_name, const u8* UNUSED(p), size_t UNUSED(size),
	DumpState state, ULONG UNUSED(num_children), const DWORD* UNUSED(children))
{
	if(!udt_should_suppress(type_name))
		return 1;

	// the data symbol is pointer-to-UDT. since we won't display its
	// contents, leave only the pointer's value.
	if(state.indirection)
		out_erase(4);	// " -> "

	// indicate something was deliberately left out
	// (otherwise, lack of output may be taken for an error)
	out(L" (..)");

	return 0;
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


static int udt_dump_normal(const wchar_t* type_name, const u8* p, size_t size,
	DumpState state, ULONG num_children, const DWORD* children)
{
	const bool fits_on_one_line = udt_fits_on_one_line(type_name, num_children, size);

	// prevent infinite recursion just to be safe (shouldn't happen)
	if(state.level >= MAX_LEVEL)
		return WDBG_NESTING_LIMIT;
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
		int err = dump_sym(child_id, el_p, state);

		// there was no output for this child; undo its indentation (if any),
		// skip everything below and proceed with the next child.
		if(err == WDBG_SUPPRESS_OUTPUT)
		{
			if(!fits_on_one_line)
				UNINDENT;
			continue;
		}

		displayed_anything = true;
		dump_error(err, el_p);	// nop if err == 0
		out(fits_on_one_line? L", " : L"\r\n");

		if(err == WDBG_SINGLE_SYMBOL_LIMIT)
			break;
	}	// for each child

	state.level--;

	if(!displayed_anything)
	{
		out_erase(2);	// "{ " or "\r\n"
		out(L"(%s)", type_name);
		return 0;
	}

	// remove trailing comma separator
	// note: we can't avoid writing it by checking if i == num_children-1:
	// each child might be the last valid data member.
	if(fits_on_one_line)
	{
		out_erase(2);	// ", "
		out(L" }");
	}

	return 0;
}


static int dump_sym_udt(DWORD type_id, const u8* p, DumpState state)
{
	ULONG64 size_ = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	const size_t size = (size_t)size_;

	// get array of child symbols (members/functions/base classes).
	DWORD num_children;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_CHILDRENCOUNT, &num_children))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	TI_FINDCHILDREN_PARAMS2 fcp(num_children);
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_FINDCHILDREN, &fcp))
		return WDBG_TYPE_INFO_UNAVAILABLE;
	num_children = fcp.p.Count;	// was truncated to MAX_CHILDREN
	const DWORD* children = fcp.p.ChildId;

	const wchar_t* type_name;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMNAME, &type_name))
		return WDBG_TYPE_INFO_UNAVAILABLE;

	int ret;
	// note: order is important (e.g. STL special-case must come before
	// suppressing UDTs, which tosses out most other C++ stdlib classes)

	ret = udt_dump_std       (type_name, p, size, state, num_children, children);
	if(ret <= 0)
		goto done;

	ret = udt_dump_suppressed(type_name, p, size, state, num_children, children);
	if(ret <= 0)
		goto done;

	ret = udt_dump_normal    (type_name, p, size, state, num_children, children);
	if(ret <= 0)
		goto done;

done:
	LocalFree((HLOCAL)type_name);
	return ret;
}


//-----------------------------------------------------------------------------


static int dump_sym_vtable(DWORD UNUSED(type_id), const u8* UNUSED(p), DumpState UNUSED(state))
{
	// unsupported (vtable internals are undocumented; too much work).
	return WDBG_SUPPRESS_OUTPUT;
}


//-----------------------------------------------------------------------------


static int dump_sym_unknown(DWORD type_id, const u8* UNUSED(p), DumpState UNUSED(state))
{
	// redundant (already done in dump_sym), but this is rare.
	DWORD type_tag;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMTAG, &type_tag))
		return WDBG_TYPE_INFO_UNAVAILABLE;

	debug_printf("Unknown tag: %d\n", type_tag);
	out(L"(unknown symbol type)");
	return 0;
}


//-----------------------------------------------------------------------------


// write name and value of the symbol <type_id> to the output buffer.
// delegates to dump_sym_* depending on the symbol's tag.
static int dump_sym(DWORD type_id, const u8* p, DumpState state)
{
	int ret = out_check_limit();
	if(ret != 0)
		return ret;

	DWORD type_tag;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMTAG, &type_tag))
		return WDBG_TYPE_INFO_UNAVAILABLE;
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
	int err = dump_sym(sym->Index, p, state);
	dump_error(err, p);
	if(err == WDBG_SUPPRESS_OUTPUT)
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
static int dump_frame_cb(const STACKFRAME64* sf, void* UNUSED(user_arg))
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
			return 0;

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
	return 1;	// keep calling
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

	int err = walk_stack(dump_frame_cb, 0, skip, (const CONTEXT*)pcontext);
	if(err != 0)
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
		DISPLAY_ERROR(L"Unable to generate minidump.");
	}

	CloseHandle(hFile);
	unlock();
}




static int wdbg_sym_shutdown()
{
	ptr_reset_visited();
	return sym_shutdown();
}


//----------------------------------------------------------------------------
// built-in self test
//----------------------------------------------------------------------------

#if PERFORM_SELF_TEST
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

	Large large_array_of_large_structs[8] = { { 0.0,0.0,0.0,0.0 } };
	Large small_array_of_large_structs[2] = { { 0.0,0.0,0.0,0.0 } };
	Small large_array_of_small_structs[8] = { { 1,2 } };
	Small small_array_of_small_structs[2] = { { 1,2 } };

	int ints[] = { 1,2,3,4,5	};
	wchar_t chars[] = { 'w','c','h','a','r','s',0 };

	//DISPLAY_ERROR(L"wdbg_sym self test: check if stack trace below is ok.");
	RaiseException(0xf001,0,0,0);
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
	Small small__ = { 0x55, 0xaa, -1 };

	struct Large
	{
		u8 large_member_u8;
		std::string large_member_string;
		double large_member_double;
	}
	large = { 0xff, "large struct string", 123456.0 };


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
	bool l_bool = true;
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


static int run_tests()
{
	test_addrs(123, 3.1415926535897932384626, "pchar string", 0xf00d);
	return 0;
}

static int dummy = run_tests();

#pragma optimize("", on)
}	// namespace test
#endif	// #if PERFORM_SELF_TEST
