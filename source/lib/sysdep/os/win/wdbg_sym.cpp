/* Copyright (c) 2010 Wildfire Games
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
 * Win32 stack trace and symbol engine.
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/wdbg_sym.h"

#include <stdlib.h>
#include <stdio.h>
#include <set>

#include "lib/byte_order.h"
#include "lib/module_init.h"
#include "lib/sysdep/cpu.h"
#include "lib/debug_stl.h"
#include "lib/app_hooks.h"
#include "lib/path_util.h"
#if ARCH_IA32
# include "lib/sysdep/arch/ia32/ia32.h"
# include "lib/sysdep/arch/ia32/ia32_asm.h"
#endif
#include "lib/external_libraries/dbghelp.h"
#include "lib/sysdep/os/win/wdbg.h"
#include "lib/sysdep/os/win/wutil.h"


#if ARCH_IA32 && !CONFIG_OMIT_FP
# define IA32_STACK_WALK_ENABLED 1
#else
# define IA32_STACK_WALK_ENABLED 0
#endif


//----------------------------------------------------------------------------
// dbghelp
//----------------------------------------------------------------------------

// passed to all dbghelp symbol query functions. we're not interested in
// resolving symbols in other processes; the purpose here is only to
// generate a stack trace. if that changes, we need to init a local copy
// of these in dump_sym_cb and pass them to all subsequent dump_*.
static HANDLE hProcess;
static uintptr_t mod_base;

#if !IA32_STACK_WALK_ENABLED
// for StackWalk64; taken from PE header by InitDbghelp.
static WORD machine;
#endif

static LibError InitDbghelp()
{
	hProcess = GetCurrentProcess();

	dbghelp_ImportFunctions();

	// set options
	// notes:
	// - can be done before SymInitialize; we do so in case
	//   any of the options affect it.
	// - do not set directly - that would zero any existing flags.
	DWORD opts = pSymGetOptions();
	opts |= SYMOPT_DEFERRED_LOADS;	// the "fastest, most efficient way"
	//opts |= SYMOPT_DEBUG;	// lots of debug spew in output window
	opts |= SYMOPT_UNDNAME;
	pSymSetOptions(opts);

	// initialize dbghelp.
	// .. request symbols from all currently active modules be loaded.
	const BOOL fInvadeProcess = TRUE;
	// .. use default *symbol* search path. we don't use this to locate
	//    our PDB file because its absolute path is stored inside the EXE.
	const PWSTR UserSearchPath = 0;
	const BOOL ok = pSymInitializeW(hProcess, UserSearchPath, fInvadeProcess);
	WARN_IF_FALSE(ok);

	mod_base = (uintptr_t)pSymGetModuleBase64(hProcess, (u64)&InitDbghelp);

#if !IA32_STACK_WALK_ENABLED
	IMAGE_NT_HEADERS* const header = pImageNtHeader((void*)(uintptr_t)mod_base);
	machine = header->FileHeader.Machine;
#endif

	return INFO::OK;
}

// ensure dbghelp is initialized exactly once.
// call every time before dbghelp functions are used.
// (on-demand initialization allows handling exceptions raised before
// winit.cpp init functions are called)
static void sym_init()
{
	static ModuleInitState initState;
	ModuleInit(&initState, InitDbghelp);
}


struct SYMBOL_INFO_PACKAGEW2 : public SYMBOL_INFO_PACKAGEW
{
	SYMBOL_INFO_PACKAGEW2()
	{
		si.SizeOfStruct = sizeof(si);
		si.MaxNameLen = MAX_SYM_NAME;
	}
};

#pragma pack(push, 1)

// note: we can't derive from TI_FINDCHILDREN_PARAMS because its members
// aren't guaranteed to precede ours (although they do in practice).
struct TI_FINDCHILDREN_PARAMS2
{
	TI_FINDCHILDREN_PARAMS2(DWORD num_children)
	{
		p.Start = 0;
		p.Count = std::min(num_children, MAX_CHILDREN);
	}

	static const DWORD MAX_CHILDREN = 300;
	TI_FINDCHILDREN_PARAMS p;
	DWORD additional_children[MAX_CHILDREN-1];
};

#pragma pack(pop)


// actual implementation; made available so that functions already under
// the lock don't have to unlock (slow) to avoid recursive locking.
static LibError ResolveSymbol_lk(void* ptr_of_interest, wchar_t* sym_name, wchar_t* file, int* line)
{
	sym_init();

	const DWORD64 addr = (DWORD64)ptr_of_interest;
	int successes = 0;

	// get symbol name (if requested)
	if(sym_name)
	{
		sym_name[0] = '\0';

		SYMBOL_INFO_PACKAGEW2 sp;
		SYMBOL_INFOW* sym = &sp.si;
		if(pSymFromAddrW(hProcess, addr, 0, sym))
		{
			wcscpy_s(sym_name, DBG_SYMBOL_LEN, sym->Name);
			successes++;
		}
	}

	// get source file and/or line number (if requested)
	if(file || line)
	{
		file[0] = '\0';
		*line = 0;

		IMAGEHLP_LINEW64 line_info = { sizeof(IMAGEHLP_LINEW64) };
		DWORD displacement; // unused but required by pSymGetLineFromAddr64!
		if(pSymGetLineFromAddrW64(hProcess, addr, &displacement, &line_info))
		{
			if(file)
			{
				// strip full path down to base name only.
				// this loses information, but that isn't expected to be a
				// problem and is balanced by not having to do this from every
				// call site (full path is too long to display nicely).
				const wchar_t* basename = path_name_only(line_info.FileName);
				wcscpy_s(file, DBG_FILE_LEN, basename);
				successes++;
			}

			if(line)
			{
				*line = line_info.LineNumber;
				successes++;
			}
		}
	}

	return (successes != 0)? INFO::OK : ERR::FAIL;
}

// read and return symbol information for the given address. all of the
// output parameters are optional; we pass back as much information as is
// available and desired. return 0 iff any information was successfully
// retrieved and stored.
// sym_name and file must hold at least the number of chars above;
// file is the base name only, not path (see rationale in wdbg_sym).
// the PDB implementation is rather slow (~500µs).
LibError debug_ResolveSymbol(void* ptr_of_interest, wchar_t* sym_name, wchar_t* file, int* line)
{
	WinScopedLock lock(WDBG_SYM_CS);
	return ResolveSymbol_lk(ptr_of_interest, sym_name, file, line);
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

#if IA32_STACK_WALK_ENABLED

static LibError ia32_walk_stack(_tagSTACKFRAME64* sf)
{
	// read previous values from _tagSTACKFRAME64
	void* prev_fp  = (void*)(uintptr_t)sf->AddrFrame .Offset;
	void* prev_ip  = (void*)(uintptr_t)sf->AddrPC    .Offset;
	void* prev_ret = (void*)(uintptr_t)sf->AddrReturn.Offset;
	if(!debug_IsStackPointer(prev_fp))
		WARN_RETURN(ERR::_11);
	if(prev_ip && !debug_IsCodePointer(prev_ip))
		WARN_RETURN(ERR::_12);
	if(prev_ret && !debug_IsCodePointer(prev_ret))
		WARN_RETURN(ERR::_13);

	// read stack frame
	void* fp       = ((void**)prev_fp)[0];
	void* ret_addr = ((void**)prev_fp)[1];
	if(!fp)
		return INFO::ALL_COMPLETE;
	if(!debug_IsStackPointer(fp))
		WARN_RETURN(ERR::_14);
	if(!debug_IsCodePointer(ret_addr))
		return ERR::FAIL;	// NOWARN (invalid address)

	void* target;
	LibError err = ia32_GetCallTarget(ret_addr, target);
	RETURN_ERR(err);
	if(target)	// were able to determine it from the call instruction
	{
		if(!debug_IsCodePointer(target))
			return ERR::FAIL;	// NOWARN (invalid address)
	}

	sf->AddrFrame .Offset = DWORD64(fp);
	sf->AddrStack .Offset = DWORD64(prev_fp)+8;	// +8 reverts effects of call + push ebp
	sf->AddrPC    .Offset = DWORD64(target);
	sf->AddrReturn.Offset = DWORD64(ret_addr);

	return INFO::OK;
}

#endif


// note: RtlCaptureStackBackTrace (http://msinilo.pl/blog/?p=40)
// is likely to be much faster than StackWalk64 (especially relevant
// for debug_GetCaller), but wasn't known during development and
// remains undocumented.

LibError wdbg_sym_WalkStack(StackFrameCallback cb, uintptr_t cbData, const CONTEXT* pcontext, const wchar_t* lastFuncToSkip)
{
	// to function properly, StackWalk64 requires a CONTEXT on
	// non-x86 systems (documented) or when in release mode (observed).
	// exception handlers can call wdbg_sym_WalkStack with their context record;
	// otherwise (e.g. dump_stack from debug_assert), we need to query it.
	CONTEXT context;
	// .. caller knows the context (most likely from an exception);
	//    since StackWalk64 may modify it, copy to a local variable.
	if(pcontext)
		context = *pcontext;
	// .. need to determine context ourselves.
	else
	{
		// there are 4 ways to do so, in order of preference:
		// - asm (easy to use but currently only implemented on IA32)
		// - RtlCaptureContext (only available on WinXP or above)
		// - intentionally raise an SEH exception and capture its context
		//   (causes annoying "first chance exception" messages and
		//   can't co-exist with WinScopedLock's destructor)
		// - GetThreadContext while suspended (a bit tricky + slow).
		//   note: it used to be common practice to query the current thread
		//   context, but WinXP SP2 and above require it be suspended.
		//
		// this MUST be done inline and not in an external function because
		// compiler-generated prolog code trashes some registers.

#if ARCH_IA32
		ia32_asm_GetCurrentContext(&context);
#else
		// we need to capture the context ASAP, lest more registers be
		// clobbered. since sym_init is no longer called from winit, the
		// best we can do is import the function pointer directly.
		static WUTIL_FUNC(pRtlCaptureContext, VOID, (PCONTEXT));
		if(!pRtlCaptureContext)
		{
			WUTIL_IMPORT_KERNEL32(RtlCaptureContext, pRtlCaptureContext);
			if(!pRtlCaptureContext)
				return ERR::NOT_SUPPORTED;	// NOWARN
		}
		memset(&context, 0, sizeof(context));
		context.ContextFlags = CONTEXT_CONTROL|CONTEXT_INTEGER;
		pRtlCaptureContext(&context);
#endif
	}
	pcontext = &context;

	_tagSTACKFRAME64 sf;
	memset(&sf, 0, sizeof(sf));
	sf.AddrPC.Mode      = AddrModeFlat;
	sf.AddrFrame.Mode   = AddrModeFlat;
	sf.AddrStack.Mode   = AddrModeFlat;
#if ARCH_AMD64
	sf.AddrPC.Offset    = pcontext->Rip;
	sf.AddrFrame.Offset = pcontext->Rbp;
	sf.AddrStack.Offset = pcontext->Rsp;
#else
	sf.AddrPC.Offset    = pcontext->Eip;
	sf.AddrFrame.Offset = pcontext->Ebp;
	sf.AddrStack.Offset = pcontext->Esp;
#endif

#if !IA32_STACK_WALK_ENABLED
	sym_init();
#endif

	// for each stack frame found:
	LibError ret  = ERR::SYM_NO_STACK_FRAMES_FOUND;
	for(;;)
	{
		// rationale:
		// - provide a separate ia32 implementation so that simple
		//   stack walks (e.g. to determine callers of malloc) do not
		//   require firing up dbghelp. that takes tens of seconds when
		//   OS symbols are installed (because symserv is wanting to access
		//   the internet), which is entirely unacceptable.
		// - VC7.1 sometimes generates stack frames despite /Oy ;
		//   ia32_walk_stack may appear to work, but it isn't reliable in
		//   this case and therefore must not be used!
		// - don't switch between ia32_stack_walk and StackWalk64 when one
		//   of them fails: this needlessly complicates things. the ia32
		//   code is authoritative provided its prerequisite (FP not omitted)
		//   is met, otherwise totally unusable.
		LibError err;
#if IA32_STACK_WALK_ENABLED
		err = ia32_walk_stack(&sf);
#else
		{
			WinScopedLock lock(WDBG_SYM_CS);

			// note: unfortunately StackWalk64 doesn't always SetLastError,
			// so we have to reset it and check for 0. *sigh*
			SetLastError(0);
			const HANDLE hThread = GetCurrentThread();
			const BOOL ok = pStackWalk64(machine, hProcess, hThread, &sf, (PVOID)pcontext, 0, pSymFunctionTableAccess64, pSymGetModuleBase64, 0);
			// note: don't use LibError_from_win32 because it raises a warning,
			// and this "fails" commonly (when no stack frames are left).
			err = ok? INFO::OK : ERR::FAIL;
		}
#endif

		// no more frames found - abort. note: also test FP because
		// StackWalk64 sometimes erroneously reports success.
		void* const fp = (void*)(uintptr_t)sf.AddrFrame.Offset;
		if(err != INFO::OK || !fp)
			return ret;

		if(lastFuncToSkip)
		{
			void* const pc = (void*)(uintptr_t)sf.AddrPC.Offset;
			wchar_t func[DBG_SYMBOL_LEN];
			err = debug_ResolveSymbol(pc, func, 0, 0);
			if(err == INFO::OK)
			{
				if(wcsstr(func, lastFuncToSkip))
					lastFuncToSkip = 0;
				continue;
			}
		}

		ret = cb(&sf, cbData);
		// callback is allowing us to continue
		if(ret == INFO::CB_CONTINUE)
			ret = INFO::OK;
		// callback reports it's done; stop calling it and return that value.
		// (can be either success or failure)
		else
		{
			debug_assert(ret <= 0);	// shouldn't return > 0
			return ret;
		}
	}
}


//
// get address of Nth function above us on the call stack (uses wdbg_sym_WalkStack)
//

// called by wdbg_sym_WalkStack for each stack frame
static LibError nth_caller_cb(const _tagSTACKFRAME64* sf, uintptr_t cbData)
{
	void** pfunc = (void**)cbData;

	// return its address
	*pfunc = (void*)(uintptr_t)sf->AddrPC.Offset;
	return INFO::OK;
}

void* debug_GetCaller(void* pcontext, const wchar_t* lastFuncToSkip)
{
	void* func;
	LibError ret = wdbg_sym_WalkStack(nth_caller_cb, (uintptr_t)&func, (const CONTEXT*)pcontext, lastFuncToSkip);
	return (ret == INFO::OK)? func : 0;
}



//-----------------------------------------------------------------------------
// helper routines for symbol value dump
//-----------------------------------------------------------------------------

// infinite recursion has never happened, but we check for it anyway.
static const size_t MAX_INDIRECTION = 255;
static const size_t MAX_LEVEL = 255;

struct DumpState
{
	size_t level;
	size_t indirection;

	DumpState()
	{
		level = 0;
		indirection = 0;
	}
};

//----------------------------------------------------------------------------

static size_t out_chars_left;
static wchar_t* out_pos;

// (only warn once until next out_init to avoid flood of messages.)
static bool out_have_warned_of_overflow;

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
// return ERR::SYM_SINGLE_SYMBOL_LIMIT once and thereafter INFO::SYM_SUPPRESS_OUTPUT.
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
	// use vswprintf, not vswprintf_s, because we want to gracefully
	// handle buffer overflows
	int len = vswprintf(out_pos, out_chars_left, fmt, args);
	va_end(args);

	// success
	if(len >= 0)
	{
		out_pos += len;
		// make sure out_chars_left remains nonnegative
		if((size_t)len > out_chars_left)
		{
			debug_assert(0);	// apparently wrote more than out_chars_left
			len = (int)out_chars_left;
		}
		out_chars_left -= len;
	}
	// no more room left
	else
	{
		// the buffer really is full yet out_chars_left may not be 0
		// (since it isn't updated if vswprintf returns -1).
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
			wcscpy_s(out_pos-ARRAY_SIZE(text), ARRAY_SIZE(text), text);	// safe
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
		return INFO::SYM_SUPPRESS_OUTPUT;
	if(out_pos - out_latched_pos > 3000)	// ~30 lines
	{
		out_have_warned_of_limit = true;
		return ERR::SYM_SINGLE_SYMBOL_LIMIT;	// NOWARN
	}

	// no limit hit, proceed normally
	return INFO::OK;
}

//----------------------------------------------------------------------------

#define INDENT STMT(for(size_t i__ = 0; i__ <= state.level; i__++) out(L"    ");)
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
// value of FP is known to dump_frame_cb (via _tagSTACKFRAME64).
// displaying variables stored in registers is out of the question;
// all we can do is display FP-relative variables.
enum CV_HREG_e
{
	CV_REG_EBP = 22,
	CV_AMD64_RSP = 335
};


static void dump_error(LibError err)
{
	switch(err)
	{
	case 0:
		// no error => no output
		break;
	case ERR::SYM_SINGLE_SYMBOL_LIMIT:
		out(L"(too much output; skipping to next top-level symbol)");
		break;
	case ERR::SYM_UNRETRIEVABLE_STATIC:
		out(L"(unavailable - located in another module)");
		break;
	case ERR::SYM_UNRETRIEVABLE:
		out(L"(unavailable)");
		break;
	case ERR::SYM_TYPE_INFO_UNAVAILABLE:
		out(L"(unavailable - type info request failed (GLE=%d))", GetLastError());
		break;
	case ERR::SYM_INTERNAL_ERROR:
		out(L"(unavailable - internal error)\r\n");
		break;
	case INFO::SYM_SUPPRESS_OUTPUT:
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
		return INFO::CANNOT_HANDLE;
	// not text
	if(!is_string(p, el_size))
		return INFO::CANNOT_HANDLE;

	wchar_t buf[512];
	if(el_size == sizeof(wchar_t))
	{
		wcsncpy(buf, (const wchar_t*)p, ARRAY_SIZE(buf)); // can't use wcscpy_s because p might be too long
		wcscpy_s(buf+ARRAY_SIZE(buf)-4, 4, L"..."); // ensure null-termination
	}
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

	out(L"\"%ls\"", buf);
	return INFO::OK;
}


// split out of dump_sequence.
static void seq_determine_formatting(size_t el_size, size_t el_count, bool* fits_on_one_line, size_t* num_elements_to_show)
{
	if(el_size == sizeof(char))
	{
		*fits_on_one_line = el_count <= 16;
		*num_elements_to_show = std::min((size_t)16u, el_count);
	}
	else if(el_size <= sizeof(int))
	{
		*fits_on_one_line = el_count <= 8;
		*num_elements_to_show = std::min((size_t)12u, el_count);
	}
	else
	{
		*fits_on_one_line = false;
		*num_elements_to_show = std::min((size_t)8u, el_count);
	}

	// make sure empty containers are displayed with [0] {}, otherwise
	// the lack of output looks like an error.
	if(!el_count)
		*fits_on_one_line = true;
}


static LibError dump_sequence(DebugStlIterator el_iterator, void* internal, size_t el_count, DWORD el_type_id, size_t el_size, DumpState state)
{
	const u8* el_p = 0;	// avoid "uninitialized" warning

	// special case: display as a string if the sequence looks to be text.
	// do this only if container isn't empty because the otherwise the
	// iterator may crash.
	if(el_count)
	{
		el_p = el_iterator(internal, el_size);

		LibError ret = dump_string(el_p, el_size);
		if(ret == INFO::OK)
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
		if(err == INFO::SYM_SUPPRESS_OUTPUT)
		{
			if(!fits_on_one_line)
				UNINDENT;
			continue;
		}

		dump_error(err);	// nop if err == INFO::OK
		// add separator unless this is the last element (can't just
		// erase below due to additional "...").
		if(i != num_elements_to_show-1)
			out(fits_on_one_line? L", " : L"\r\n");

		if(err == ERR::SYM_SINGLE_SYMBOL_LIMIT)
			break;
	}	// for each child

	// indicate some elements were skipped
	if(el_count != num_elements_to_show)
		out(L" ...");

	state.level--;
	if(fits_on_one_line)
		out(L" }");
	return INFO::OK;
}


static const u8* array_iterator(void* internal, size_t el_size)
{
	const u8*& pos = *(const u8**)internal;
	const u8* cur_pos = pos;
	pos += el_size;
	return cur_pos;
}


static LibError dump_array(const u8* p, size_t el_count, DWORD el_type_id, size_t el_size, DumpState state)
{
	const u8* iterator_internal_pos = p;
	return dump_sequence(array_iterator, &iterator_internal_pos,
		el_count, el_type_id, el_size, state);
}


static const _tagSTACKFRAME64* current_stackframe64;

static LibError CanHandleDataKind(DWORD dataKind)
{
	switch(dataKind)
	{
	case DataIsMember:
		// address is already correct (udt_dump_normal retrieved the offset;
		// we do it that way so we can check it against the total
		// UDT size for safety) and SymFromIndex would fail
		return INFO::SKIPPED;

	case DataIsUnknown:
		WARN_RETURN(ERR::FAIL);

	case DataIsStaticMember:
		// this symbol is defined as static in another module =>
		// there's nothing we can do.
		return ERR::SYM_UNRETRIEVABLE_STATIC;	// NOWARN

	case DataIsLocal:
	case DataIsStaticLocal:
	case DataIsParam:
	case DataIsObjectPtr:
	case DataIsFileStatic:
	case DataIsGlobal:
	case DataIsConstant:
		// ok, can handle
		return INFO::OK;
	}

	WARN_RETURN(ERR::LOGIC);	// UNREACHABLE
}

static bool IsRelativeToFramePointer(DWORD flags, DWORD reg)
{
	if(flags & SYMFLAG_FRAMEREL)	// note: this is apparently obsolete
		return true;
	if((flags & SYMFLAG_REGREL) == 0)
		return false;
	if(reg == CV_REG_EBP || reg == CV_AMD64_RSP)
		return true;
	return false;
}

static bool IsUnretrievable(DWORD flags)
{
	// note: it is unlikely that the crashdump register context
	// contains the correct values for this scope, so symbols
	// stored in or relative to a general register are unavailable.
	if(flags & SYMFLAG_REGISTER)
		return true;

	// note: IsRelativeToFramePointer is called first, so if we still
	// see this flag, the base register is not the frame pointer.
	// since we most probably don't know its value in the current
	// scope (see above), the symbol is inaccessible.
	if(flags & SYMFLAG_REGREL)
		return true;

	return false;
}

static LibError DetermineSymbolAddress(DWORD id, const SYMBOL_INFOW* sym, const u8** pp)
{
	const _tagSTACKFRAME64* sf = current_stackframe64;

	DWORD dataKind;
	if(!pSymGetTypeInfo(hProcess, mod_base, id, TI_GET_DATAKIND, &dataKind))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	LibError ret = CanHandleDataKind(dataKind);
	RETURN_ERR(ret);
	if(ret == INFO::SKIPPED)
		return INFO::OK;	// pp is already correct

	// note: we have not yet observed a non-zero TI_GET_ADDRESSOFFSET or
	// TI_GET_ADDRESS, and TI_GET_OFFSET is apparently equal to sym->Address.

	// get address
	uintptr_t addr = (uintptr_t)sym->Address;
	if(IsRelativeToFramePointer(sym->Flags, sym->Register))
	{
#if ARCH_AMD64
		addr += (uintptr_t)sf->AddrStack.Offset;
#else
		addr += (uintptr_t)sf->AddrFrame.Offset;
# if defined(NDEBUG)
		// NB: the addresses of register-relative symbols are apparently
		// incorrect [VC8, 32-bit Wow64]. the problem occurs regardless of
		// IA32_STACK_WALK_ENABLED and with both ia32_asm_GetCurrentContext
		// and RtlCaptureContext. the EBP, ESP and EIP values returned by
		// ia32_asm_GetCurrentContext match those reported by the IDE, so
		// the problem appears to lie in the offset values stored in the PDB.
		if(sym->Flags & SYMFLAG_PARAMETER)
			addr += sizeof(void*);
		else
			addr += sizeof(void*) * 2;
# endif
#endif
	}
	else if(IsUnretrievable(sym->Flags))
		return ERR::SYM_UNRETRIEVABLE;	// NOWARN

	*pp = (const u8*)(uintptr_t)addr;

	debug_printf(L"SYM| %ls at %p  flags=%X dk=%d sym->addr=%I64X fp=%I64x\n", sym->Name, *pp, sym->Flags, dataKind, sym->Address, sf->AddrFrame.Offset);
	return INFO::OK;
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
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	// get element count and size
	DWORD el_type_id = 0;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &el_type_id))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	// .. workaround: TI_GET_COUNT returns total struct size for
	//    arrays-of-struct. therefore, calculate as size / el_size.
	ULONG64 el_size_;
	if(!pSymGetTypeInfo(hProcess, mod_base, el_type_id, TI_GET_LENGTH, &el_size_))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	const size_t el_size = (size_t)el_size_;
	debug_assert(el_size != 0);
	const size_t num_elements = size/el_size;
	debug_assert(num_elements != 0);
 
	return dump_array(p, num_elements, el_type_id, el_size, state);
}


//-----------------------------------------------------------------------------

// if the current value is a printable character, display in that form.
// this isn't only done in btChar because characters are sometimes stored
// in integers.
static void AppendCharacterIfPrintable(u64 data)
{
	if(data < 0x100)
	{
		int c = (int)data;
		if(isprint(c))
			out(L" ('%hc')", c);
	}
}


static LibError dump_sym_base_type(DWORD type_id, const u8* p, DumpState state)
{
	DWORD base_type;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_BASETYPE, &base_type))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	ULONG64 size_ = 0;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	// single out() call. note: we pass a single u64 for all sizes,
	// which will only work on little-endian systems.
	// must be declared before goto to avoid W4 warning.
	const wchar_t* fmt = L"";

	u64 data = movzx_le64(p, size);
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
	// floating-point
	case btFloat:
		if(size == sizeof(float))
		{
			// NB: the C calling convention calls for float arguments to be
			// converted to double. passing `data' wouldn't work because it's
			// merely a zero-extended 32-bit representation of the float.
			float value;
			memcpy(&value, p, sizeof(value));
			out(L"%f (0x%08I64X)", value, data);
		}
		else if(size == sizeof(double))
			out(L"%g (0x%016I64X)", data, data);
		else
			debug_assert(0);	// invalid float size
		break;

	// boolean
	case btBool:
		debug_assert(size == sizeof(bool));
		if(data == 0 || data == 1)
			out(L"%ls", data? L"true " : L"false");
		else
			out(L"(bool)0x%02I64X", data);
		break;

	// integers (displayed as decimal and hex)
	// note: 0x00000000 can get annoying (0 would be nicer),
	// but it indicates the variable size and makes for consistently
	// formatted structs/arrays. (0x1234 0 0x5678 is ugly)
	case btInt:
	case btLong:
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
			fmt = L"%I64d (0x%02I64X)";
		}
		else if(size == 2)
			fmt = L"%I64d (0x%04I64X)";
		else if(size == 4)
			fmt = L"%I64d (0x%08I64X)";
		else if(size == 8)
			fmt = L"%I64d (0x%016I64X)";
		else
			debug_assert(0);	// invalid size for integers
		out(fmt, data, data);
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
		out(L"%d", data);
		AppendCharacterIfPrintable(data);
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
			debug_assert(0);	// non-pointer btVoid or btNoType
		break;

	default:
		debug_assert(0);	// unknown type
		break;

	// unsupported complex types
	case btBCD:
	case btCurrency:
	case btDate:
	case btVariant:
	case btComplex:
	case btBit:
	case btBSTR:
	case btHresult:
		return ERR::SYM_UNSUPPORTED;	// NOWARN
	}

	return INFO::OK;
}


//-----------------------------------------------------------------------------

static LibError dump_sym_base_class(DWORD type_id, const u8* p, DumpState state)
{
	DWORD base_class_type_id;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &base_class_type_id))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);

	// this is a virtual base class. we can't display those because it'd
	// require reading the VTbl, which is difficult given lack of documentation
	// and just not worth it.
	DWORD vptr_ofs;
	if(pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_VIRTUALBASEPOINTEROFFSET, &vptr_ofs))
		return ERR::SYM_UNSUPPORTED;	// NOWARN

	return dump_sym(base_class_type_id, p, state);
}


//-----------------------------------------------------------------------------

static LibError dump_sym_data(DWORD id, const u8* p, DumpState state)
{
	SYMBOL_INFO_PACKAGEW2 sp;
	SYMBOL_INFOW* sym = &sp.si;
	if(!pSymFromIndexW(hProcess, mod_base, id, sym))
		RETURN_ERR(ERR::SYM_TYPE_INFO_UNAVAILABLE);

	out(L"%ls = ", sym->Name);

	__try
	{
		RETURN_ERR(DetermineSymbolAddress(id, sym, &p));
		// display value recursively
		return dump_sym(sym->TypeIndex, p, state);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return ERR::SYM_INTERNAL_ERROR;	// NOWARN
	}
}


//-----------------------------------------------------------------------------

static LibError dump_sym_enum(DWORD type_id, const u8* p, DumpState UNUSED(state))
{
	ULONG64 size_ = 0;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	const i64 enum_value = movsx_le64(p, size);

	// get array of child symbols (enumerants).
	DWORD num_children;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_CHILDRENCOUNT, &num_children))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	TI_FINDCHILDREN_PARAMS2 fcp(num_children);
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_FINDCHILDREN, &fcp))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	num_children = fcp.p.Count;	// was truncated to MAX_CHILDREN
	const DWORD* children = fcp.p.ChildId;

	// for each child (enumerant):
	for(size_t i = 0; i < num_children; i++)
	{
		DWORD child_data_id = children[i];

		// get this enumerant's value. we can't make any assumptions about
		// the variant's type or size  - no restriction is documented.
		// rationale: VariantChangeType is much less tedious than doing
		// it manually and guarantees we cover everything. the OLE DLL is
		// already pulled in by e.g. OpenGL anyway.
		VARIANT v;
		if(!pSymGetTypeInfo(hProcess, mod_base, child_data_id, TI_GET_VALUE, &v))
			WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
		if(VariantChangeType(&v, &v, 0, VT_I8) != S_OK)
			continue;

		// it's the one we want - output its name.
		if(enum_value == v.llVal)
		{
			const wchar_t* name;
			if(!pSymGetTypeInfo(hProcess, mod_base, child_data_id, TI_GET_SYMNAME, &name))
				WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
			out(L"%ls", name);
			LocalFree((HLOCAL)name);
			return INFO::OK;
		}
	}

	// we weren't able to retrieve a matching enum value, but can still
	// produce reasonable output (the numeric value).
	// note: could goto here after a SGTI fails, but we fail instead
	// to make sure those errors are noticed.
	out(L"%I64d", enum_value);
	return INFO::OK;
}


//-----------------------------------------------------------------------------

static LibError dump_sym_function(DWORD UNUSED(type_id), const u8* UNUSED(p), DumpState UNUSED(state))
{
	return INFO::SYM_SUPPRESS_OUTPUT;
}


//-----------------------------------------------------------------------------

static LibError dump_sym_function_type(DWORD UNUSED(type_id), const u8* p, DumpState state)
{
	// this symbol gives class parent, return type, and parameter count.
	// unfortunately the one thing we care about, its name,
	// isn't exposed via TI_GET_SYMNAME, so we resolve it ourselves.

	wchar_t name[DBG_SYMBOL_LEN];
	LibError err = ResolveSymbol_lk((void*)p, name, 0, 0);

	if(state.indirection == 0)
		out(L"0x%p ", p);
	if(err == INFO::OK)
		out(L"(%ls)", name);
	return INFO::OK;
}


//-----------------------------------------------------------------------------

// do not follow pointers that we have already displayed. this reduces
// clutter a bit and prevents infinite recursion for cyclical references
// (e.g. via struct S { S* p; } s; s.p = &s;)

// note: allocating memory dynamically would cause trouble if dumping
// the stack from within memory-related code (the allocation hook would
// be reentered, which is not permissible).

static const size_t maxVisited = 1000;
static const u8* visited[maxVisited];
static size_t numVisited;

static void ptr_reset_visited()
{
	numVisited = 0;
}

static bool ptr_already_visited(const u8* p)
{
	for(size_t i = 0; i < numVisited; i++)
	{
		if(visited[i] == p)
			return true;
	}

	if(numVisited < maxVisited)
	{
		visited[numVisited] = p;
		numVisited++;
	}
	// capacity exceeded
	else
	{
		// warn user - but only once (we can't use the regular
		// debug_DisplayError and wdbg_assert doesn't have a
		// suppress mechanism)
		static bool haveComplained;
		if(!haveComplained)
		{
			debug_printf(L"WARNING: ptr_already_visited: capacity exceeded, increase maxVisited\n");
			debug_break();
			haveComplained = true;
		}
	}

	return false;
}


static LibError dump_sym_pointer(DWORD type_id, const u8* p, DumpState state)
{
	ULONG64 size_ = 0;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	// read+output pointer's value.
	p = (const u8*)(uintptr_t)movzx_le64(p, size);
	out(L"0x%p", p);

	// bail if it's obvious the pointer is bogus
	// (=> can't display what it's pointing to)
	if(debug_IsPointerBogus(p))
		return INFO::OK;

	// avoid duplicates and circular references
	if(ptr_already_visited(p))
	{
		out(L" (see above)");
		return INFO::OK;
	}

	// display what the pointer is pointing to.
	// if the pointer is invalid (despite "bogus" check above),
	// dump_data_sym recovers via SEH and prints an error message.
	// if the pointed-to value turns out to uninteresting (e.g. void*),
	// the responsible dump_sym* will erase "->", leaving only address.
	out(L" -> ");
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &type_id))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);

	// prevent infinite recursion just to be safe (shouldn't happen)
	if(state.indirection >= MAX_INDIRECTION)
		WARN_RETURN(ERR::SYM_NESTING_LIMIT);
	state.indirection++;
	return dump_sym(type_id, p, state);
}


//-----------------------------------------------------------------------------


static LibError dump_sym_typedef(DWORD type_id, const u8* p, DumpState state)
{
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &type_id))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	return dump_sym(type_id, p, state);
}


//-----------------------------------------------------------------------------


// determine type and size of the given child in a UDT.
// useful for UDTs that contain typedefs describing their contents,
// e.g. value_type in STL containers.
static LibError udt_get_child_type(const wchar_t* child_name, ULONG num_children, const DWORD* children, DWORD* el_type_id, size_t* el_size)
{
	const DWORD lastError = GetLastError();

	*el_type_id = 0;
	*el_size = 0;

	for(ULONG i = 0; i < num_children; i++)
	{
		const DWORD child_id = children[i];

		SYMBOL_INFO_PACKAGEW2 sp;
		SYMBOL_INFOW* sym = &sp.si;
		if(!pSymFromIndexW(hProcess, mod_base, child_id, sym))
		{
			// this happens for several UDTs; cause is unknown.
			debug_assert(GetLastError() == ERROR_NOT_FOUND);
			continue;
		}
		if(!wcscmp(sym->Name, child_name))
		{
			*el_type_id = sym->TypeIndex;
			*el_size = (size_t)sym->Size;
			return INFO::OK;
		}
	}

	SetLastError(lastError);

	// (happens if called for containers that are treated as STL but are not)
	return ERR::SYM_CHILD_NOT_FOUND;	// NOWARN
}


static LibError udt_dump_std(const wchar_t* type_name, const u8* p, size_t size, DumpState state, ULONG num_children, const DWORD* children)
{
	LibError err;

	// not a C++ standard library object; can't handle it.
	if(wcsncmp(type_name, L"std::", 5) != 0)
		return INFO::CANNOT_HANDLE;

	// check for C++ objects that should be displayed via udt_dump_normal.
	// STL containers are special-cased and the rest (apart from those here)
	// are ignored, because for the most part they are spew.
	if(!wcsncmp(type_name, L"std::pair", 9))
		return INFO::CANNOT_HANDLE;

	// display contents of STL containers
	// .. get element type
	DWORD el_type_id;
	size_t el_size;
 	err = udt_get_child_type(L"value_type", num_children, children, &el_type_id, &el_size);
	if(err != INFO::OK)
		goto not_valid_container;
	// .. get iterator and # elements
	size_t el_count;
	DebugStlIterator el_iterator;
	u8 it_mem[DEBUG_STL_MAX_ITERATOR_SIZE];
	err = debug_stl_get_container_info(type_name, p, size, el_size, &el_count, &el_iterator, it_mem);
	if(err != INFO::OK)
		goto not_valid_container;
	return dump_sequence(el_iterator, it_mem, el_count, el_type_id, el_size, state);
not_valid_container:

	// build and display detailed "error" message.
	wchar_t buf[100];
	const wchar_t* text;
	// .. object named std::* but doesn't include a "value_type" child =>
	//    it's a non-STL C++ stdlib object. wasn't handled by the
	//    special case above, so we just display its simplified type name
	//    (the contents are usually spew).
	if(err == ERR::SYM_CHILD_NOT_FOUND)
		text = L"";
	// .. not one of the containers we can analyse.
	if(err == ERR::STL_CNT_UNKNOWN)
		text = L"unsupported ";
	// .. container of a known type but contents are invalid.
	if(err == ERR::STL_CNT_INVALID)
		text = L"uninitialized/invalid ";
	// .. some other error encountered
	else
	{
		swprintf_s(buf, ARRAY_SIZE(buf), L"error %d while analyzing ", err);
		text = buf;
	}

	// (debug_stl modifies its input string in-place; type_name is
	// a const string returned by dbghelp)
	wchar_t type_name_buf[DBG_SYMBOL_LEN];
	wcscpy_s(type_name_buf, ARRAY_SIZE(type_name_buf), type_name);

	out(L"(%ls%ls)", text, debug_stl_simplify_name(type_name_buf));
	return INFO::OK;
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


static LibError udt_dump_suppressed(const wchar_t* type_name, const u8* UNUSED(p), size_t UNUSED(size), DumpState state, ULONG UNUSED(num_children), const DWORD* UNUSED(children))
{
	if(!udt_should_suppress(type_name))
		return INFO::CANNOT_HANDLE;

	// the data symbol is pointer-to-UDT. since we won't display its
	// contents, leave only the pointer's value.
	if(state.indirection)
		out_erase(4);	// " -> "

	// indicate something was deliberately left out
	// (otherwise, lack of output may be taken for an error)
	out(L" (..)");

	return INFO::OK;
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


static LibError udt_dump_normal(const wchar_t* type_name, const u8* p, size_t size, DumpState state, ULONG num_children, const DWORD* children)
{
	const bool fits_on_one_line = udt_fits_on_one_line(type_name, num_children, size);

	// prevent infinite recursion just to be safe (shouldn't happen)
	if(state.level >= MAX_LEVEL)
		WARN_RETURN(ERR::SYM_NESTING_LIMIT);
	state.level++;

	out(fits_on_one_line? L"{ " : L"\r\n");

	bool displayed_anything = false;
	for(ULONG i = 0; i < num_children; i++)
	{
		const DWORD child_id = children[i];

		// get offset. if not available, skip this child
		// (we only display data here, not e.g. typedefs)
		DWORD ofs = 0;
		if(!pSymGetTypeInfo(hProcess, mod_base, child_id, TI_GET_OFFSET, &ofs))
			continue;
		if(ofs >= size)
		{
			debug_printf(L"INVALID_UDT %ls %d %d\n", type_name, ofs, size);
		}
		//debug_assert(ofs < size);

		if(!fits_on_one_line)
			INDENT;

		const u8* el_p = p+ofs;
		LibError err = dump_sym(child_id, el_p, state);

		// there was no output for this child; undo its indentation (if any),
		// skip everything below and proceed with the next child.
		if(err == INFO::SYM_SUPPRESS_OUTPUT)
		{
			if(!fits_on_one_line)
				UNINDENT;
			continue;
		}

		displayed_anything = true;
		dump_error(err);	// nop if err == INFO::OK
		out(fits_on_one_line? L", " : L"\r\n");

		if(err == ERR::SYM_SINGLE_SYMBOL_LIMIT)
			break;
	}	// for each child

	state.level--;

	if(!displayed_anything)
	{
		out_erase(2);	// "{ " or "\r\n"
		out(L"(%ls)", type_name);
		return INFO::OK;
	}

	// remove trailing comma separator
	// note: we can't avoid writing it by checking if i == num_children-1:
	// each child might be the last valid data member.
	if(fits_on_one_line)
	{
		out_erase(2);	// ", "
		out(L" }");
	}

	return INFO::OK;
}


static LibError dump_sym_udt(DWORD type_id, const u8* p, DumpState state)
{
	ULONG64 size_ = 0;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	// get array of child symbols (members/functions/base classes).
	DWORD num_children;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_CHILDRENCOUNT, &num_children))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	TI_FINDCHILDREN_PARAMS2 fcp(num_children);
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_FINDCHILDREN, &fcp))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	num_children = fcp.p.Count;	// was truncated to MAX_CHILDREN
	const DWORD* children = fcp.p.ChildId;

	const wchar_t* type_name;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMNAME, &type_name))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);

	LibError ret;
	// note: order is important (e.g. STL special-case must come before
	// suppressing UDTs, which tosses out most other C++ stdlib classes)

	ret = udt_dump_std       (type_name, p, size, state, num_children, children);
	if(ret != INFO::CANNOT_HANDLE)
		goto done;

	ret = udt_dump_suppressed(type_name, p, size, state, num_children, children);
	if(ret != INFO::CANNOT_HANDLE)
		goto done;

	ret = udt_dump_normal    (type_name, p, size, state, num_children, children);
	if(ret != INFO::CANNOT_HANDLE)
		goto done;

done:
	LocalFree((HLOCAL)type_name);
	return ret;
}


//-----------------------------------------------------------------------------


static LibError dump_sym_vtable(DWORD UNUSED(type_id), const u8* UNUSED(p), DumpState UNUSED(state))
{
	// unsupported (vtable internals are undocumented; too much work).
	return INFO::SYM_SUPPRESS_OUTPUT;
}


//-----------------------------------------------------------------------------


static LibError dump_sym_unknown(DWORD type_id, const u8* UNUSED(p), DumpState UNUSED(state))
{
	// redundant (already done in dump_sym), but this is rare.
	DWORD type_tag;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMTAG, &type_tag))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);

	debug_printf(L"SYM| unknown tag: %d\n", type_tag);
	out(L"(unknown symbol type)");
	return INFO::OK;
}


//-----------------------------------------------------------------------------


// write name and value of the symbol <type_id> to the output buffer.
// delegates to dump_sym_* depending on the symbol's tag.
static LibError dump_sym(DWORD type_id, const u8* p, DumpState state)
{
	RETURN_ERR(out_check_limit());

	DWORD type_tag;
	if(!pSymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMTAG, &type_tag))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
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


//-----------------------------------------------------------------------------
// stack trace
//-----------------------------------------------------------------------------

struct IMAGEHLP_STACK_FRAME2 : public IMAGEHLP_STACK_FRAME
{
	IMAGEHLP_STACK_FRAME2(const _tagSTACKFRAME64* sf)
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

static bool ShouldSkipSymbol(const wchar_t* name)
{
	if(!wcscmp(name, L"suppress__"))
		return true;
	if(!wcscmp(name, L"__profile"))
		return true;
	return false;
}

// output the symbol's name and value via dump_sym*.
// called from dump_frame_cb for each local symbol; lock is held.
static BOOL CALLBACK dump_sym_cb(SYMBOL_INFOW* sym, ULONG UNUSED(size), void* UNUSED(ctx))
{
	if(ShouldSkipSymbol(sym->Name))
		return TRUE;	// continue

	out_latch_pos();	// see decl
	mod_base = (uintptr_t)sym->ModBase;
	const u8* p = (const u8*)(uintptr_t)sym->Address;
	DumpState state;

	INDENT;
	LibError err = dump_sym(sym->Index, p, state);
	dump_error(err);
	if(err == INFO::SYM_SUPPRESS_OUTPUT)
		UNINDENT;
	else
		out(L"\r\n");

	return TRUE;	// continue
}

// called by wdbg_sym_WalkStack for each stack frame
static LibError dump_frame_cb(const _tagSTACKFRAME64* sf, uintptr_t UNUSED(cbData))
{
	current_stackframe64 = sf;
	void* func = (void*)(uintptr_t)sf->AddrPC.Offset;

	wchar_t func_name[DBG_SYMBOL_LEN]; wchar_t file[DBG_FILE_LEN]; int line;
	LibError ret = ResolveSymbol_lk(func, func_name, file, &line);
	if(ret == INFO::OK)
	{
		// don't trace back further than the app's entry point
		// (no one wants to see this frame). checking for the
		// function name isn't future-proof, but not stopping is no big deal.
		// an alternative would be to check if module=kernel32, but
		// that would cut off callbacks as well.
		// note: the stdcall mangled name includes parameter size, which is
		// different in 64-bit, so only check the first characters.
		if(!wcsncmp(func_name, L"_BaseProcessStart", 17))
			return INFO::OK;

		out(L"%ls (%ls:%d)\r\n", func_name, file, line);
	}
	else
		out(L"%p\r\n", func);

	// only enumerate symbols for this stack frame
	// (i.e. its locals and parameters)
	// problem: debug info is scope-aware, so we won't see any variables
	// declared in sub-blocks. we'd have to pass an address in that block,
	// which isn't worth the trouble. since 
	IMAGEHLP_STACK_FRAME2 imghlp_frame(sf);
	const PIMAGEHLP_CONTEXT context = 0;	// ignored
	pSymSetContext(hProcess, &imghlp_frame, context);

	const ULONG64 base = 0; const wchar_t* const mask = 0;	// use scope set by pSymSetContext
	pSymEnumSymbolsW(hProcess, base, mask, dump_sym_cb, 0);

	out(L"\r\n");
	return INFO::CB_CONTINUE;
}


LibError debug_DumpStack(wchar_t* buf, size_t maxChars, void* pcontext, const wchar_t* lastFuncToSkip)
{
	static intptr_t busy;
	if(!cpu_CAS(&busy, 0, 1))
		return ERR::REENTERED;	// NOWARN

	out_init(buf, maxChars);
	ptr_reset_visited();

	LibError ret = wdbg_sym_WalkStack(dump_frame_cb, 0, (const CONTEXT*)pcontext, lastFuncToSkip);

	busy = 0;
	COMPILER_FENCE;

	return ret;
}


//-----------------------------------------------------------------------------

// write out a "minidump" containing register and stack state; this enables
// examining the crash in a debugger. called by wdbg_exception_filter.
// heavily modified from http://www.codeproject.com/debug/XCrashReportPt3.asp
// lock must be held.
void wdbg_sym_WriteMinidump(EXCEPTION_POINTERS* exception_pointers)
{
	sym_init();

	WinScopedLock lock(WDBG_SYM_CS);

	fs::wpath path = ah_get_log_dir()/L"crashlog.dmp";
	HANDLE hFile = CreateFileW(path.string().c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		DEBUG_DISPLAY_ERROR(L"wdbg_sym_WriteMinidump: unable to create crashlog.dmp.");
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = GetCurrentThreadId();
	mei.ExceptionPointers = exception_pointers;
	mei.ClientPointers = FALSE;
	// exception_pointers is not in our address space.

	// note: we don't store other crashlog info within the dump file
	// (UserStreamParam), since we will need to generate a plain text file on
	// non-Windows platforms. users will just have to send us both files.

	HANDLE hProcess = GetCurrentProcess();
	DWORD pid = GetCurrentProcessId();
	if(!pMiniDumpWriteDump || !pMiniDumpWriteDump(hProcess, pid, hFile, MiniDumpNormal, &mei, 0, 0))
		DEBUG_DISPLAY_ERROR(L"wdbg_sym_WriteMinidump: unable to generate minidump.");

	CloseHandle(hFile);
}
