/**
 * =========================================================================
 * File        : wdbg_sym.cpp
 * Project     : 0 A.D.
 * Description : Win32 stack trace and symbol engine.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "wdbg_sym.h"

#include <stdlib.h>
#include <stdio.h>
#include <set>

#include "lib/byte_order.h"
#include "lib/sysdep/cpu.h"

#include "lib/debug_stl.h"
#include "lib/app_hooks.h"
#include "lib/os_path.h"
#include "lib/path_util.h"
#if ARCH_IA32
# include "lib/sysdep/ia32/ia32.h"
# include "lib/sysdep/ia32/ia32_asm.h"
#endif
#include "lib/external_libraries/dbghelp.h"
#include "winit.h"
#include "wutil.h"


WINIT_REGISTER_CRITICAL_INIT(wdbg_sym_Init);


//----------------------------------------------------------------------------
// dbghelp
//----------------------------------------------------------------------------

// passed to all dbghelp symbol query functions. we're not interested in
// resolving symbols in other processes; the purpose here is only to
// generate a stack trace. if that changes, we need to init a local copy
// of these in dump_sym_cb and pass them to all subsequent dump_*.
static HANDLE hProcess;
static uintptr_t mod_base;

// for StackWalk64; taken from PE header by wdbg_init.
static WORD machine;

// call on-demand (allows handling exceptions raised before winit.cpp
// init functions are called); no effect if already initialized.
static LibError sym_init()
{
	// bail if already initialized (there's nothing to do).
	// don't use pthread_once because we need to return success/error code.
	static uintptr_t already_initialized = 0;
	if(!cpu_CAS(&already_initialized, 0, 1))
		return INFO::OK;

	hProcess = GetCurrentProcess();

	// set options
	// notes:
	// - can be done before SymInitialize; we do so in case
	//   any of the options affect it.
	// - do not set directly - that would zero any existing flags.
	DWORD opts = SymGetOptions();
	opts |= SYMOPT_DEFERRED_LOADS;	// the "fastest, most efficient way"
	//opts |= SYMOPT_DEBUG;	// lots of debug spew in output window
	opts |= SYMOPT_UNDNAME;
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
	IMAGE_NT_HEADERS* header = ImageNtHeader((void*)(uintptr_t)mod_base);
	machine = header->FileHeader.Machine;

	return INFO::OK;
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
		p.Count = std::min(num_children, MAX_CHILDREN);
	}

	static const DWORD MAX_CHILDREN = 400;
	TI_FINDCHILDREN_PARAMS p;
	DWORD additional_children[MAX_CHILDREN-1];
};


// actual implementation; made available so that functions already under
// the lock don't have to unlock (slow) to avoid recursive locking.
static LibError debug_resolve_symbol_lk(void* ptr_of_interest, char* sym_name, char* file, int* line)
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
		if(SymFromAddrW(hProcess, addr, 0, sym))
		{
			wsprintfA(sym_name, "%ws", sym->Name);
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
				const char* base_name = path_name_only(line_info.FileName);
				wsprintf(file, "%s", base_name);
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
LibError debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line)
{
	WinScopedLock lock(WDBG_SYM_CS);
	return debug_resolve_symbol_lk(ptr_of_interest, sym_name, file, line);
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

#if ARCH_IA32 && !CONFIG_OMIT_FP

static LibError ia32_walk_stack(_tagSTACKFRAME64* sf)
{
	// read previous values from _tagSTACKFRAME64
	void* prev_fp  = (void*)(uintptr_t)sf->AddrFrame .Offset;
	void* prev_ip  = (void*)(uintptr_t)sf->AddrPC    .Offset;
	void* prev_ret = (void*)(uintptr_t)sf->AddrReturn.Offset;
	if(!debug_is_stack_ptr(prev_fp))
		WARN_RETURN(ERR::_11);
	if(prev_ip && !debug_is_code_ptr(prev_ip))
		WARN_RETURN(ERR::_12);
	if(prev_ret && !debug_is_code_ptr(prev_ret))
		WARN_RETURN(ERR::_13);

	// read stack frame
	void* fp       = ((void**)prev_fp)[0];
	void* ret_addr = ((void**)prev_fp)[1];
	if(!fp)
		return INFO::ALL_COMPLETE;
	if(!debug_is_stack_ptr(fp))
		WARN_RETURN(ERR::_14);
	if(!debug_is_code_ptr(ret_addr))
		return ERR::FAIL;	// NOWARN (invalid address)

	void* target;
	LibError err = ia32_GetCallTarget(ret_addr, &target);
	RETURN_ERR(err);
	if(target)	// were able to determine it from the call instruction
	{
		if(!debug_is_code_ptr(target))
			return ERR::FAIL;	// NOWARN (invalid address)
	}

	sf->AddrFrame .Offset = (DWORD64)fp;
	sf->AddrPC    .Offset = (DWORD64)target;
	sf->AddrReturn.Offset = (DWORD64)ret_addr;

	return INFO::OK;
}

#endif	// #if ARCH_IA32 && !CONFIG_OMIT_FP


static void skip_this_frame(size_t& skip, void* context)
{
	if(!context)
		skip++;
}


typedef VOID (*PRtlCaptureContext)(PCONTEXT);
static PRtlCaptureContext s_RtlCaptureContext;

LibError wdbg_sym_WalkStack(StackFrameCallback cb, uintptr_t cbData, size_t skip, const CONTEXT* pcontext)
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
		skip_this_frame(skip, (void*)pcontext);

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

#if ARCH_IA32
		ia32_asm_GetCurrentContext(&context);
#else
		// we no longer bother supporting stack walks on pre-WinXP systems
		// that lack RtlCaptureContext. at least importing dynamically does
		// allow the application to run on such systems.
		// (note: the RaiseException method is annoying due to output in
		// the debugger window and interaction with WinScopedLock's dtor)
		if(!s_RtlCaptureContext)
			return ERR::SYM_UNSUPPORTED;	// NOWARN
		s_RtlCaptureContext(&context);
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

	// for each stack frame found:
	LibError ret  = ERR::SYM_NO_STACK_FRAMES_FOUND;
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
#if ARCH_IA32 && !CONFIG_OMIT_FP
		err = ia32_walk_stack(&sf);
#else
		WinScopedLock lock(WDBG_SYM_CS);
		sym_init();
		// note: unfortunately StackWalk64 doesn't always SetLastError,
		// so we have to reset it and check for 0. *sigh*
		SetLastError(0);
		const HANDLE hThread = GetCurrentThread();
		BOOL ok = StackWalk64(machine, hProcess, hThread, &sf, (PVOID)pcontext, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0);
		// note: don't use LibError_from_win32 because it raises a warning,
		// and this "fails" commonly (when no stack frames are left).
		err = ok? INFO::OK : ERR::FAIL;
#endif

		// no more frames found - abort. note: also test FP because
		// StackWalk64 sometimes erroneously reports success.
		void* fp = (void*)(uintptr_t)sf.AddrFrame.Offset;
		if(err != INFO::OK || !fp)
			return ret;

		if(skip)
		{
			skip--;
			continue;
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

void* debug_get_nth_caller(size_t skip, void* pcontext)
{
	void* func;
	skip_this_frame(skip, pcontext);
	LibError ret = wdbg_sym_WalkStack(nth_caller_cb, (uintptr_t)&func, skip, (const CONTEXT*)pcontext);
	return (ret == INFO::OK)? func : 0;
}



//-----------------------------------------------------------------------------
// helper routines for symbol value dump
//-----------------------------------------------------------------------------

// overflow is impossible in practice, but check for robustness.
// keep in sync with DumpState.
static const size_t MAX_INDIRECTION = 255;
static const size_t MAX_LEVEL = 255;

struct DumpState
{
	// keep in sync with MAX_* above
	size_t level : 8;
	size_t indirection : 8;

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

#define INDENT STMT(for(size_t i = 0; i <= state.level; i++) out(L"    ");)
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
	case ERR::SYM_SINGLE_SYMBOL_LIMIT:
		out(L"(too much output; skipping to next top-level symbol)");
		break;
	case ERR::SYM_UNRETRIEVABLE_STATIC:
		out(L"(unavailable - located in another module)");
		break;
	case ERR::SYM_UNRETRIEVABLE_REG:
		out(L"(unavailable - stored in register %s)", string_for_register((CV_HREG_e)(uintptr_t)p));
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
	return INFO::OK;
}


// split out of dump_sequence.
static void seq_determine_formatting(size_t el_size, size_t el_count,
	bool* fits_on_one_line, size_t* num_elements_to_show)
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


static LibError dump_sequence(DebugStlIterator el_iterator, void* internal,
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

		dump_error(err, el_p);	// nop if err == INFO::OK
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


static LibError dump_array(const u8* p,
	size_t el_count, DWORD el_type_id, size_t el_size, DumpState state)
{
	const u8* iterator_internal_pos = p;
	return dump_sequence(array_iterator, &iterator_internal_pos,
		el_count, el_type_id, el_size, state);
}


static const _tagSTACKFRAME64* current_stackframe64;

static LibError determine_symbol_address(DWORD id, DWORD UNUSED(type_id), const u8** pp)
{
	const _tagSTACKFRAME64* sf = current_stackframe64;

	DWORD data_kind;
	if(!SymGetTypeInfo(hProcess, mod_base, id, TI_GET_DATAKIND, &data_kind))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	switch(data_kind)
	{
	// SymFromIndex will fail
	case DataIsMember:
		// pp is already correct (udt_dump_normal retrieved the offset;
		// we do it that way so we can check it against the total
		// UDT size for safety).
		return INFO::OK;

	// this symbol is defined as static in another module =>
	// there's nothing we can do.
	case DataIsStaticMember:
		return ERR::SYM_UNRETRIEVABLE_STATIC;	// NOWARN

	// ok; will handle below
	case DataIsLocal:
	case DataIsStaticLocal:
	case DataIsParam:
	case DataIsObjectPtr:
	case DataIsFileStatic:
	case DataIsGlobal:
		break;

	NODEFAULT;
	}

	// get SYMBOL_INFO (we need .Flags)
	SYMBOL_INFO_PACKAGEW2 sp;
	SYMBOL_INFOW* sym = &sp.si;
	if(!SymFromIndexW(hProcess, mod_base, id, sym))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);

DWORD addrofs = 0;
ULONG64 addr2 = 0;
DWORD ofs2 = 0;
SymGetTypeInfo(hProcess, mod_base, id, TI_GET_ADDRESSOFFSET, &addrofs);
SymGetTypeInfo(hProcess, mod_base, id, TI_GET_ADDRESS, &addr2);
SymGetTypeInfo(hProcess, mod_base, id, TI_GET_OFFSET, &ofs2);



	// get address
	uintptr_t addr = sym->Address;
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
		return ERR::SYM_UNRETRIEVABLE_REG;	// NOWARN
	}

	*pp = (const u8*)(uintptr_t)addr;

debug_printf("SYM| %ws at %p  flags=%X dk=%d sym->addr=%I64X addrofs=%X addr2=%I64X ofs2=%X\n", sym->Name, *pp, sym->Flags, data_kind, sym->Address, addrofs, addr2, ofs2);

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
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	// get element count and size
	DWORD el_type_id = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &el_type_id))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	// .. workaround: TI_GET_COUNT returns total struct size for
	//    arrays-of-struct. therefore, calculate as size / el_size.
	ULONG64 el_size_;
	if(!SymGetTypeInfo(hProcess, mod_base, el_type_id, TI_GET_LENGTH, &el_size_))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
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
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	ULONG64 size_ = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
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
				debug_assert(0);	// invalid float size
			fmt = L"%g";
			break;

		// signed integers (displayed as decimal)
		case btInt:
		case btLong:
			if(size != 1 && size != 2 && size != 4 && size != 8)
				debug_assert(0);	// invalid int size
			// need to re-load and sign-extend, because we output 64 bits.
			data = movsx_le64(p, size);
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
				debug_assert(0);	// invalid size_t size
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
				debug_assert(0);	// non-pointer btVoid or btNoType
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
			return ERR::SYM_UNSUPPORTED;	// NOWARN
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

	return INFO::OK;
}


//-----------------------------------------------------------------------------

static LibError dump_sym_base_class(DWORD type_id, const u8* p, DumpState state)
{
	DWORD base_class_type_id;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &base_class_type_id))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);

	// this is a virtual base class. we can't display those because it'd
	// require reading the VTbl, which is difficult given lack of documentation
	// and just not worth it.
	DWORD vptr_ofs;
	if(SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_VIRTUALBASEPOINTEROFFSET, &vptr_ofs))
		return ERR::SYM_UNSUPPORTED;	// NOWARN

	return dump_sym(base_class_type_id, p, state);
	

}


//-----------------------------------------------------------------------------

static LibError dump_sym_data(DWORD id, const u8* p, DumpState state)
{
	// display name (of variable/member)
	const wchar_t* name;
	if(!SymGetTypeInfo(hProcess, mod_base, id, TI_GET_SYMNAME, &name))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	out(L"%s = ", name);
	LocalFree((HLOCAL)name);

	__try
	{
		// get type_id and address
		DWORD type_id;
		if(!SymGetTypeInfo(hProcess, mod_base, id, TI_GET_TYPEID, &type_id))
			WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
		RETURN_ERR(determine_symbol_address(id, type_id, &p));

		// display value recursively
		return dump_sym(type_id, p, state);
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
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	const i64 enum_value = movsx_le64(p, size);

	// get array of child symbols (enumerants).
	DWORD num_children;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_CHILDRENCOUNT, &num_children))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	TI_FINDCHILDREN_PARAMS2 fcp(num_children);
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_FINDCHILDREN, &fcp))
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
		if(!SymGetTypeInfo(hProcess, mod_base, child_data_id, TI_GET_VALUE, &v))
			WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
		if(VariantChangeType(&v, &v, 0, VT_I8) != S_OK)
			continue;

		// it's the one we want - output its name.
		if(enum_value == v.llVal)
		{
			const wchar_t* name;
			if(!SymGetTypeInfo(hProcess, mod_base, child_data_id, TI_GET_SYMNAME, &name))
				WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
			out(L"%s", name);
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

static LibError dump_sym_function(DWORD UNUSED(type_id), const u8* UNUSED(p),
	DumpState UNUSED(state))
{
	return INFO::SYM_SUPPRESS_OUTPUT;
}


//-----------------------------------------------------------------------------

static LibError dump_sym_function_type(DWORD UNUSED(type_id), const u8* p, DumpState UNUSED(state))
{
	// this symbol gives class parent, return type, and parameter count.
	// unfortunately the one thing we care about, its name,
	// isn't exposed via TI_GET_SYMNAME, so we resolve it ourselves.

	char name[DBG_SYMBOL_LEN];
	LibError err = debug_resolve_symbol_lk((void*)p, name, 0, 0);

	out(L"0x%p", p);
	if(err == INFO::OK)
		out(L" (%hs)", name);
	return INFO::OK;
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

// called by debug_dump_stack but not during shutdown (this must remain valid
// until the very end to allow crash reports)
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
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	// read+output pointer's value.
	p = (const u8*)(uintptr_t)movzx_le64(p, size);
	out(L"0x%p", p);

	// bail if it's obvious the pointer is bogus
	// (=> can't display what it's pointing to)
	if(debug_is_pointer_bogus(p))
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
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &type_id))
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
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_TYPEID, &type_id))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
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
			WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);

		ULONG64 size;
		if(!SymGetTypeInfo(hProcess, mod_base, child_id, TI_GET_LENGTH, &size))
			WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);

		*el_type_id = type_id;
		*el_size = (size_t)size;
		return INFO::OK;
	}

	// (happens if called for containers that are treated as STL but are not)
	return ERR::SYM_CHILD_NOT_FOUND;	// NOWARN
}


static LibError udt_dump_std(const wchar_t* wtype_name, const u8* p, size_t size, DumpState state,
	ULONG num_children, const DWORD* children)
{
	LibError err;

	// not a C++ standard library object; can't handle it.
	if(wcsncmp(wtype_name, L"std::", 5) != 0)
		return INFO::CANNOT_HANDLE;

	// check for C++ objects that should be displayed via udt_dump_normal.
	// STL containers are special-cased and the rest (apart from those here)
	// are ignored, because for the most part they are spew.
	if(!wcsncmp(wtype_name, L"std::pair", 9))
		return INFO::CANNOT_HANDLE;

	// convert to char since debug_stl doesn't support wchar_t.
	char ctype_name[DBG_SYMBOL_LEN];
	sprintf_s(ctype_name, ARRAY_SIZE(ctype_name), "%ws", wtype_name);

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
	err = debug_stl_get_container_info(ctype_name, p, size, el_size, &el_count, &el_iterator, it_mem);
	if(err != INFO::OK)
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
	if(err == ERR::SYM_CHILD_NOT_FOUND)
		text = "";
	// .. not one of the containers we can analyse.
	if(err == ERR::STL_CNT_UNKNOWN)
		text = "unsupported ";
	// .. container of a known type but contents are invalid.
	if(err == ERR::STL_CNT_INVALID)
		text = "uninitialized/invalid ";
	// .. some other error encountered
	else
	{
		sprintf_s(buf, ARRAY_SIZE(buf), "error %d while analyzing ", err);
		text = buf;
	}
	out(L"(%hs%hs)", text, debug_stl_simplify_name(ctype_name));
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


static LibError udt_dump_suppressed(const wchar_t* type_name, const u8* UNUSED(p), size_t UNUSED(size),
	DumpState state, ULONG UNUSED(num_children), const DWORD* UNUSED(children))
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


static LibError udt_dump_normal(const wchar_t* type_name, const u8* p, size_t size,
	DumpState state, ULONG num_children, const DWORD* children)
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
		if(!SymGetTypeInfo(hProcess, mod_base, child_id, TI_GET_OFFSET, &ofs))
			continue;
		debug_assert(ofs < size);

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
		dump_error(err, el_p);	// nop if err == INFO::OK
		out(fits_on_one_line? L", " : L"\r\n");

		if(err == ERR::SYM_SINGLE_SYMBOL_LIMIT)
			break;
	}	// for each child

	state.level--;

	if(!displayed_anything)
	{
		out_erase(2);	// "{ " or "\r\n"
		out(L"(%s)", type_name);
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
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_LENGTH, &size_))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	const size_t size = (size_t)size_;

	// get array of child symbols (members/functions/base classes).
	DWORD num_children;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_CHILDRENCOUNT, &num_children))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	TI_FINDCHILDREN_PARAMS2 fcp(num_children);
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_FINDCHILDREN, &fcp))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);
	num_children = fcp.p.Count;	// was truncated to MAX_CHILDREN
	const DWORD* children = fcp.p.ChildId;

	const wchar_t* type_name;
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMNAME, &type_name))
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
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMTAG, &type_tag))
		WARN_RETURN(ERR::SYM_TYPE_INFO_UNAVAILABLE);

	debug_printf("SYM| unknown tag: %d\n", type_tag);
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
	if(!SymGetTypeInfo(hProcess, mod_base, type_id, TI_GET_SYMTAG, &type_tag))
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

// output the symbol's name and value via dump_sym*.
// called from dump_frame_cb for each local symbol; lock is held.
static BOOL CALLBACK dump_sym_cb(SYMBOL_INFO* sym, ULONG UNUSED(size), void* UNUSED(ctx))
{
	out_latch_pos();	// see decl
	mod_base = sym->ModBase;
	const u8* p = (const u8*)(uintptr_t)sym->Address;
	DumpState state;

	INDENT;
	LibError err = dump_sym(sym->Index, p, state);
	dump_error(err, p);
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

	char func_name[DBG_SYMBOL_LEN]; char file[DBG_FILE_LEN]; int line;
	LibError ret = debug_resolve_symbol_lk(func, func_name, file, &line);
	if(ret == INFO::OK)
	{
		// don't trace back further than the app's entry point
		// (no one wants to see this frame). checking for the
		// function name isn't future-proof, but not stopping is no big deal.
		// an alternative would be to check if module=kernel32, but
		// that would cut off callbacks as well.
		if(!strcmp(func_name, "_BaseProcessStart@4"))
			return INFO::OK;

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

	const ULONG64 base = 0; const char* const mask = 0;	// use scope set by SymSetContext
	SymEnumSymbols(hProcess, base, mask, dump_sym_cb, 0);

	out(L"\r\n");
	return INFO::CB_CONTINUE;
}


LibError debug_dump_stack(wchar_t* buf, size_t max_chars, size_t skip, void* pcontext)
{
	static uintptr_t already_in_progress;
	if(!cpu_CAS(&already_in_progress, 0, 1))
		return ERR::REENTERED;	// NOWARN

	out_init(buf, max_chars);
	ptr_reset_visited();

	skip_this_frame(skip, pcontext);
	LibError ret = wdbg_sym_WalkStack(dump_frame_cb, 0, skip, (const CONTEXT*)pcontext);

	already_in_progress = 0;

	return ret;
}


//-----------------------------------------------------------------------------

// write out a "minidump" containing register and stack state; this enables
// examining the crash in a debugger. called by wdbg_exception_filter.
// heavily modified from http://www.codeproject.com/debug/XCrashReportPt3.asp
// lock must be held.
void wdbg_sym_WriteMinidump(EXCEPTION_POINTERS* exception_pointers)
{
	WinScopedLock lock(WDBG_SYM_CS);

	OsPath path = OsPath(ah_get_log_dir())/"crashlog.dmp";
	HANDLE hFile = CreateFile(path.string().c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
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

	HANDLE hProcess = GetCurrentProcess(); DWORD pid = GetCurrentProcessId();
	if(!MiniDumpWriteDump(hProcess, pid, hFile, MiniDumpNormal, &mei, 0, 0))
		DEBUG_DISPLAY_ERROR(L"wdbg_sym_WriteMinidump: unable to generate minidump.");

	CloseHandle(hFile);
}


//-----------------------------------------------------------------------------

static LibError wdbg_sym_Init()
{
	HMODULE hKernel32Dll = GetModuleHandle("kernel32.dll");
	s_RtlCaptureContext = (PRtlCaptureContext)GetProcAddress(hKernel32Dll, "RtlCaptureContext");

	return INFO::OK;
}
