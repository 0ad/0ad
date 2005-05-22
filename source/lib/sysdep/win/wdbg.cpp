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

#ifdef I18N
#include "PS/i18n.h"
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


static void set_exception_handler();


//
// globals, set by wdbg_init
//

// our instance (= load address = 0x400000 on Win32). used by
// CreateDialogParam to locate the "program error" dialog resource.
static HINSTANCE hInstance;

// passed to all dbghelp symbol query functions. we're not interested in
// resolving symbols in other processes; the purpose here is only to
// generate a stack trace. if that changes, we need to init a local copy
// of these in dump_sym_cb and pass them to all subsequent dump_*.
static HANDLE hProcess;
static ULONG64 mod_base;

// for StackWalk64; taken from PE header by wdbg_init
static WORD machine;


static int wdbg_init()
{
	// we don't want wrap the contents of main() in a __try block
	// (platform-specific), and regular C++ exceptions don't catch
	// everything. therefore, install an unhandled exception filter here.
	// it won't be called if the program is being debugged, so to test it,
	// wrap the call to main() in win.cpp!WinMain in a __try block.
	set_exception_handler();

	hProcess = GetCurrentProcess();
	hInstance = GetModuleHandle(0);

	SymSetOptions(SYMOPT_DEFERRED_LOADS);
	SymInitialize(hProcess, 0, TRUE);

	mod_base = SymGetModuleBase64(hProcess, (u64)&wdbg_init);
	IMAGE_NT_HEADERS* header = ImageNtHeader((void*)mod_base);
	machine = header->FileHeader.Machine;

	return 0;
}


static int wdbg_shutdown(void)
{
	SymCleanup(hProcess);
	return 0;
}


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
//
// we don't reuse similar code from the profiler in wcpu.cpp:
// it is designed to interrupt the main thread periodically,
// whereas what we need here is to interrupt any thread on-demand.

// parameter passing to helper thread. currently static storage,
// but the struct simplifies switching to a queue later.
static struct BreakInfo
{
	// real (not pseudo) handle of thread whose context we will change.
	HANDLE hThread;

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
// called from brk_thread_func; return error code as void*.
static void* brk_disable_all_in_ctx(BreakInfo* bi, CONTEXT* context)
{
	context->Dr7 &= ~brk_all_local_enables;
	return 0;	// success
}


// find a free register, set type according to <bi> and
// mark it as enabled in <context>.
// called from brk_thread_func; return error code as void*.
static void* brk_enable_in_ctx(BreakInfo* bi, CONTEXT* context)
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
	return (void*)(intptr_t)ERR_LIMIT;
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
	return 0;	// success
}


// carry out the request stored in the BreakInfo* parameter.
// return error code as void*.
static void* brk_thread_func(void* arg)
{
	DWORD err;
	void* ret;
	BreakInfo* bi = (BreakInfo*)arg;

	err = SuspendThread(bi->hThread);
	// abort, since GetThreadContext only works if the target is suspended.
	if(err == (DWORD)-1)
	{
		debug_warn("brk_thread_func: SuspendThread failed");
		goto fail;
	}
	// target is now guaranteed to be suspended,
	// since the Windows counter never goes negative.

	//////////////////////////////////////////////////////////////////////////

	// to avoid deadlock, be VERY CAREFUL to avoid anything that may block,
	// including locks taken by the OS (e.g. malloc, GetProcAddress).
	{

	CONTEXT context;
	context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	if(!GetThreadContext(bi->hThread, &context))
	{
		debug_warn("brk_thread_func: GetThreadContext failed");
		goto fail;
	}

#if defined(_M_IX86)
	if(bi->want_all_disabled)
		ret = brk_disable_all_in_ctx(bi, &context);
	else
		ret = brk_enable_in_ctx     (bi, &context);

	if(!SetThreadContext(bi->hThread, &context))
	{
		debug_warn("brk_thread_func: SetThreadContext failed");
		goto fail;
	}
#else
#error "port"
#endif

	}

	//////////////////////////////////////////////////////////////////////////

	err = ResumeThread(bi->hThread);
	assert(err != 0);

	return ret;

fail:
	return (void*)(intptr_t)-1;
}


// latch current thread and carry out the request stored in brk_info.
static int brk_run_thread()
{
	int err;
	BreakInfo* bi = &brk_info;

	// we need a real HANDLE to the target thread for use with
	// Suspend|ResumeThread and GetThreadContext.
	// alternative: DuplicateHandle on the current thread pseudo-HANDLE.
	// this way is a bit more obvious/simple.
	const DWORD access = THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_SUSPEND_RESUME;
	bi->hThread = OpenThread(access, FALSE, GetCurrentThreadId());
	if(bi->hThread == INVALID_HANDLE_VALUE)
	{
		debug_warn("debug_set_break: OpenThread failed");
		return -1;
	}

	pthread_t thread;
	err = pthread_create(&thread, 0, brk_thread_func, bi);
	assert2(err == 0);

	void* ret;
	err = pthread_join(thread, &ret);
	assert2(err == 0 && ret == 0);

	return (int)(intptr_t)ret;
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
	int ret = brk_run_thread();

	unlock();
	return ret;
}


// remove all breakpoints that were set by debug_set_break.
// important, since these are a limited resource.
int debug_remove_all_breaks()
{
	lock();

	brk_info.want_all_disabled = true;
	int ret = brk_run_thread();
	brk_info.want_all_disabled = false;

	unlock();
	return ret;
}


//////////////////////////////////////////////////////////////////////////////
//
// dbghelp support routines for walking the stack
//
//////////////////////////////////////////////////////////////////////////////

// iterate over a call stack.
// if <thread_context> != 0, we start there; otherwise, at the current
// stack frame. call <cb> for each stack frame found, also passing the
// user-specified <ctx>. if it returns <= 0, we stop immediately and
// pass on that value; otherwise, the eventual return value is -1
// ("callback never succeeded").
//
// note: can't just pass function's address to the callback -
// dump_frame_cb needs the frame pointer for reg-relative variables.
static int walk_stack(int (*cb)(STACKFRAME64*, void*), void* ctx, CONTEXT* thread_context = 0)
{
	HANDLE hThread = GetCurrentThread();

	// we need to set STACKFRAME64.AddrPC and AddrFrame for the initial
	// StackWalk call in our loop. if the caller passed in a thread context
	// (e.g. if calling from an exception handler), we use that; otherwise,
	// determine the current PC / frame pointer ourselves.
	// GetThreadContext is documented not to work if the current thread
	// is running, but that seems to be widespread practice. regardless, we
	// avoid using it, since simple asm code is safer. deliberately raising
	// an exception to retrieve the CONTEXT is too slow (due to jump to
	// ring0), since this is called from mmgr for each allocation.
	STACKFRAME64 frame;
	memset(&frame, 0, sizeof(frame));

#if defined(_M_AMD64)

	DWORD64 rip_, rbp_;
	if(thread_context)
	{
		rip_ = thread_context->Rip;
		rbp_ = thread_context->Rbp;
	}
	else
	{
		__asm
		{
		cur_rip:
			mov		rax, offset cur_rip
			mov		[rip_], rax
			mov		[rbp_], rbp
		}
	}

	frame.AddrPC.Offset    = rip_;
	frame.AddrPC.Mode      = AddrModeFlat;
	frame.AddrFrame.Offset = rbp_;
	frame.AddrFrame.Mode   = AddrModeFlat;

#elif defined(_M_IX86)

	DWORD eip_, ebp_;
	if(thread_context)
	{
		eip_ = thread_context->Eip;
		ebp_ = thread_context->Ebp;
	}
	else
	{
		__asm
		{
		cur_eip:
			mov		eax, offset cur_eip
			mov		[eip_], eax
			mov		[ebp_], ebp
		}
	}

	frame.AddrPC.Offset    = eip_;
	frame.AddrPC.Mode      = AddrModeFlat;
	frame.AddrFrame.Offset = ebp_;
	frame.AddrFrame.Mode   = AddrModeFlat;

#else

#error "TODO: set STACKFRAME64.AddrPC, AddrFrame for this platform"

#endif

	// for each stack frame found:
	for(;;)
	{
		lock();
		BOOL ok = StackWalk64(machine, hProcess, hThread, &frame, thread_context, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0);
		unlock();

		// no more frames found, and callback never succeeded; abort.
		if(!ok)
			return -1;

		void* func = (void*)frame.AddrPC.Offset;

		int ret = cb(&frame, ctx);
		// callback reports it's done; stop calling it and return that value.
		// (can be 0 for success, or a negative error code)
		if(ret <= 0)
			return ret;
	}
}



// ~500µs
int debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line)
{
	int successes = 0;

	lock();

	{
	const DWORD64 addr = (DWORD64)ptr_of_interest;

	// get symbol name
	sym_name[0] = '\0';
	SYMBOL_INFO_PACKAGE sp;
	SYMBOL_INFO* sym = &sp.si;
	sym->SizeOfStruct = sizeof(sp.si);
	sym->MaxNameLen = MAX_SYM_NAME;
	if(SymFromAddr(hProcess, addr, 0, sym))
	{
		sprintf(sym_name, "%s", sym->Name);
		successes++;
	}

	// get source file + line number
	file[0] = '\0';
	*line = 0;
	IMAGEHLP_LINE64 line_info = { sizeof(IMAGEHLP_LINE64) };
	DWORD displacement; // unused but required by SymGetLineFromAddr64!
	if(SymGetLineFromAddr64(hProcess, addr, &displacement, &line_info))
	{
		sprintf(file, "%s", line_info.FileName);
		*line = line_info.LineNumber;
		successes++;
	}

	}

	unlock();
	return (successes == 0)? -1 : 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// get address of Nth function above us on the call stack (uses walk_stack)
//
//////////////////////////////////////////////////////////////////////////////

struct NthCallerParams
{
	int left_to_skip;
	void* func;
};

// called by walk_stack for each stack frame
static int nth_caller_cb(STACKFRAME64* frame, void* ctx)
{
	NthCallerParams* p = (NthCallerParams*)ctx;

	// not the one we want yet
	if(p->left_to_skip > 0)
	{
		p->left_to_skip--;
		return 1;	// keep calling
	}

	// return its address
	p->func = (void*)frame->AddrPC.Offset;
	return 0;
}


// n starts at 1
void* debug_get_nth_caller(uint n)
{
	NthCallerParams params = { (int)n-1 + 3, 0 };
		// skip walk_stack, debug_get_nth_caller and its caller
	if(walk_stack(nth_caller_cb, &params) == 0)
		return params.func;
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// helper routines for symbol value dump
//
//////////////////////////////////////////////////////////////////////////////

static const size_t DUMP_BUF_SIZE = 64000;
static wchar_t buf[DUMP_BUF_SIZE];	// buffer for stack trace
static wchar_t* pos;			// current pos in buf

static void out(const wchar_t* fmt, ...)
{
	// Don't overflow the buffer (and abort if we're about to)
	if (pos-buf+1000 > DUMP_BUF_SIZE)
	{
		debug_warn("");
		return;
	};

	va_list args;
	va_start(args, fmt);
	pos += vswprintf(pos, 1000, fmt, args);
	va_end(args);
}


// does it look like an ASCII string is located at <addr>?
// set <stride> to 2 to search for WCS-2 strings (of western characters!).
// called by dump_ptr and dump_array for their string special-cases.
//
// algorithm: scan the "string" and count # text chars vs. garbage.
static bool is_string_ptr(u64 addr, size_t stride = 1)
{
	// completely bogus on IA-32; save ourselves the segfault (slow).
#ifdef _M_IX86
	if(addr < 0x10000 || addr > 0xc0000000)
		return false;
#endif

	const char* str = (const char*)addr;

	__try
	{
		int score = 0;

		for(;;)
		{
			// current character is:
			const int c = *str & 0xff;	// prevent sign extension
			// .. text
			if(isalnum(c))
				score += 2;
			// .. end of string
			else if(!c)
				break;
			// .. garbage
			else if(!isprint(c))
				score -= 5;

			// too much garbage found, probably not a string.
			// abort fairly early so we don't segfault unnecessarily (slow).
			if(score <= -10)
				break;

			str += stride;
		}

		return (score > 0);
	}
	__except(1)
	{
		debug_printf("^ raised by is_string_ptr; ignore\n");
		return false;
	}
}


// zero-extend <size> (truncated to 8) bytes of little-endian data to u64,
// starting at address <p> (need not be aligned).
static u64 movzx_64le(const u8* p, size_t size)
{
	if(size > 8)
		size = 8;

	u64 data = 0;
	for(u64 i = 0; i < MIN(size,8); i++)
		data |= ((u64)p[i]) << (i*8);

	return data;
}


// sign-extend <size> (truncated to 8) bytes of little-endian data to i64,
// starting at address <p> (need not be aligned).
static i64 movsx_64le(const u8* p, size_t size)
{
	if(size > 8)
		size = 8;

	u64 data = movzx_64le(p, size);

	// no point in sign-extending if >= 8 bytes were requested
	if(size < 8)
	{
		u64 sign_bit = 1;
		sign_bit <<= (size*8)-1;
			// be sure that we don't shift more than variable's bit width

		// number would be negative in the smaller type,
		// so sign-extend, i.e. set all more significant bits.
		if(data & sign_bit)
		{
			const u64 size_mask = (sign_bit+sign_bit)-1;
			data |= ~size_mask;
		}
	}

	return (i64)data;
}


//////////////////////////////////////////////////////////////////////////////
//
// output values of specific types of local variables
//
//////////////////////////////////////////////////////////////////////////////

// forward decl; called by dump_udt.
static int dump_data_sym(DWORD data_idx, const u8* p, uint level);

// forward decl; called by dump_array.
static int dump_type_sym(DWORD type_idx, const u8* p, uint level);


// these functions return -1 if they're not able to produce any reasonable
// output; dump_type_sym will display value as "?"


// <type_id> is a SymTagPointerType; output its value.
// called by dump_type_sym; lock is held.
static int dump_ptr(const u8* p, size_t size)
{
	const u64 data = movzx_64le(p, size);

	const wchar_t* fmt;

	// char*
	if(is_string_ptr(data, sizeof(char)))
		fmt = L"\"%hs\"";
	// WCHAR*
	else if(is_string_ptr(data, sizeof(WCHAR)))
		fmt = L"\"%s\"";
	// generic 32-bit pointer
	else if(size == 4)
		fmt = L"0x%08X";
	// generic 64-bit pointer
	else
		fmt = L"0x%I64016X";
	
	out(fmt, data);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////


// <type_id> is a SymTagBaseType; output its value.
// called by dump_type_sym; lock is held.
static int dump_base_type(DWORD type_idx, const u8* p, size_t size, uint level)
{
	UNUSED(level);

	DWORD base_type;
	if(!SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_BASETYPE, &base_type))
		return -1;

	u64 data = movzx_64le(p, size);

	// single out() call
	// (note: passing pointers in u64 assumes little-endian)
	const wchar_t* fmt;

	switch(base_type)
	{
		case btBool:
			assert(size == sizeof(bool));
			fmt = L"%hs";
			data = (u64)(data? "true " : "false");
			break;

		case btFloat:
			if(size == sizeof(float))
				fmt = L"%f";
			else if(size == sizeof(double))
				fmt = L"%lf";
			else
				assert(0);
			break;

		// signed integers
		case btInt:
		case btLong:
			data = movsx_64le(p, size);
			if(size == 1 || size == 2 || size == 4)
				fmt = L"%d";
			else if(size == 8)
				fmt = L"%I64d";
			else
				assert(0);
			break;

		// unsigned integers (displayed as hex)
		case btUInt:
		case btULong:
			if(size == 1)
				fmt = L"0x%02X";
			else if(size == 2)
				fmt = L"0x%04X";
			else if(size == 4)
				fmt = L"0x%08X";
			else if(size == 8)
				fmt = L"0x%016I64X";
			else
				assert(0);
			break;

		// either 8-bit integer or character (character value appended below)
		case btChar:
			assert(size == sizeof(char));
			fmt = L"%I64d";
			break;

		case btWChar:
			assert(size == sizeof(wchar_t));
			fmt = L"%c";
			break;

		// shouldn't happen
		case btNoType:
		case btVoid:
		default:
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


// <type_id> is a SymTagEnum; output its value.
// called by dump_type_sym; lock is held.
static int dump_enum(DWORD type_idx, const u8* p, size_t size, uint level)
{
	UNUSED(level);

	const i64 current_value = movsx_64le(p, size);

	DWORD num_children;
	if(!SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_CHILDRENCOUNT, &num_children))
		goto name_unavailable;

	// alloc an array to hold child IDs
	const size_t MAX_CHILDREN = 1000;
	char child_buf[sizeof(TI_FINDCHILDREN_PARAMS)+MAX_CHILDREN*sizeof(DWORD)];
	TI_FINDCHILDREN_PARAMS* fcp = (TI_FINDCHILDREN_PARAMS*)child_buf;
	fcp->Start = 0;
	fcp->Count = MIN(num_children, MAX_CHILDREN);

	if(!SymGetTypeInfo(hProcess, mod_base, type_idx, TI_FINDCHILDREN, fcp))
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


// <type_id> is a SymTagArrayType; output its value.
// called by dump_type_sym; lock is held.
static int dump_array(DWORD type_idx, const u8* p, size_t size, uint level)
{
	DWORD elements;
	if(!SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_COUNT, &elements))
		return -1;

	DWORD el_type_idx = 0;
	if(!SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_TYPEID, &el_type_idx))
		return -1;

	size_t stride = (size_t)(size / elements);


	//
	// special case for character arrays, i.e. strings
	//

	u64 addr = (u64)p;
	// .. char[]
	if(stride == sizeof(char) && is_string_ptr(addr, stride))
	{
		out(L"\"%hs\"", p);
		return 0;
	}
	// .. WCHAR[] (don't use wchar_t, since that might be 4 bytes)
	if(stride == sizeof(WCHAR) && is_string_ptr(addr, stride))
	{
		out(L"\"%s\"", p);
		return 0;
	}


	//
	// regular array output
	//

	int err = 0;

	out(L"{ ");

	const uint elements_to_show = MIN(32, elements);
	for(uint i = 0; i < elements_to_show; i++)
	{
		int ret = dump_type_sym(el_type_idx, p + i*stride, level+1);
		// skip trailing comma
		if(i != elements_to_show-1)
			out(L", ");

		// remember first error
		if(err == 0)
			err = ret;
	}
	// we truncated some
	if(elements != elements_to_show)
		out(L" ...");

	out(L" }");
	return err;
}


//////////////////////////////////////////////////////////////////////////////


// <type_id> is a SymTagUDT; output its value.
// called by dump_type_sym; lock is held.
static int dump_udt(DWORD type_idx, const u8* p, size_t size, uint level)
{
	UNUSED(size);

	out(L"\r\n");

	DWORD num_children;
	if(!SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_CHILDRENCOUNT, &num_children))
		return -1;

	// alloc an array to hold child IDs
	const size_t MAX_CHILDREN = 1000;
	char child_buf[sizeof(TI_FINDCHILDREN_PARAMS)+MAX_CHILDREN*sizeof(DWORD)];
	TI_FINDCHILDREN_PARAMS* fcp = (TI_FINDCHILDREN_PARAMS*)child_buf;
	fcp->Start = 0;
	fcp->Count = MIN(num_children, MAX_CHILDREN);

	if(!SymGetTypeInfo(hProcess, mod_base, type_idx, TI_FINDCHILDREN, fcp))
		return -1;

	int err = 0;

	// recursively display each child (call back to dump_data)
	for(uint i = 0; i < fcp->Count; i++)
	{
		DWORD child_data_idx = fcp->ChildId[i];

		DWORD ofs = 0;
		if(!SymGetTypeInfo(hProcess, mod_base, child_data_idx, TI_GET_OFFSET, &ofs))
			// this happens if child_data_idx doesn't represent a member
			// variable of the UDT - e.g. a SymTagBaseClass. we don't bother
			// checking the tag; just skip this symbol.
			continue;

		int ret = dump_data_sym(child_data_idx, p+ofs, level+1);

		// remember first error
		if(err == 0)
			err = ret;
	}

	return err;
}


//////////////////////////////////////////////////////////////////////////////
//
// stack trace
//
//////////////////////////////////////////////////////////////////////////////

// given a data symbol's type identifier, output its type name (if
// applicable), determine what kind of variable it describes, and
// call the appropriate dump_* routine.
//
// split out of dump_data_sym so we can recurse for typedefs (cleaner than
// 'restart' via goto or loop). lock is held.
static int dump_type_sym(DWORD type_idx, const u8* p, uint level)
{
	DWORD type_tag;
	if(!SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_SYMTAG, &type_tag))
		return -1;

	// get "type name" (only available for SymTagUDT, SymTagEnum, and
	// SymTagTypedef types).
	// note: can't use SymFromIndex to get tag as well as name, because it
	// fails when name isn't available (e.g. if this is a SymTagBaseType).
	WCHAR* type_name;
	if(SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_SYMNAME, &type_name))
	{
		out(L"(%s)", type_name);
		LocalFree(type_name);
	}

	ULONG64 size;
	if(!SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_LENGTH, &size))
		return -1;


	switch(type_tag)
	{
	case SymTagPointerType:
		return dump_ptr(p, (size_t)size);

	case SymTagEnum:
		return dump_enum(type_idx, p, (size_t)size, level);

	case SymTagBaseType:
		return dump_base_type(type_idx, p, (size_t)size, level);

	case SymTagUDT:
		return dump_udt(type_idx, p, (size_t)size, level);

	case SymTagArrayType:
		return dump_array(type_idx, p, (size_t)size, level);

	case SymTagTypedef:
		if(!SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_TYPEID, &type_idx))
			return -1;
		return dump_type_sym(type_idx, p, level);

	default:
		debug_printf("Unknown tag: %d\n", type_tag);
		return -1;
	}
}


//////////////////////////////////////////////////////////////////////////////


// indent to current nesting level, display name, and output value via
// dump_type_sym.
//
// split out of dump_sym_cb so dump_udt can call back here for its members.
// lock is held.
static int dump_data_sym(DWORD data_idx, const u8* p, uint level)
{
	// note: return both type_idx and name in one call for convenience.
	// this is also more efficient than TI_GET_SYMNAME (avoids 1 LocalAlloc).
	SYMBOL_INFO_PACKAGEW sp;
	SYMBOL_INFOW* sym = &sp.si;
	sym->SizeOfStruct = sizeof(sp.si);
	sym->MaxNameLen = MAX_SYM_NAME;
	if(!SymFromIndexW(hProcess, mod_base, data_idx, sym))
		return -1;

	// dump_udt does some filtering (it skips symbols that don't have a
	// defined offset), but we still get some SymTagBaseClass here.
	// just ignore them. note: dump_sym_cb is the only other call site.
	if(sym->Tag != SymTagData)
		return 0;

	// indent
	for(uint i = 0; i <= level+1; i++)
		out(L"    ");

	out(L"%s = ", sym->Name);

	int ret;
	__try
	{
		ret = dump_type_sym(sym->TypeIndex, p, level);
		// couldn't produce any reasonable output; value = "?"
		if(ret < 0)
			out(L"?");
	}
	__except(1)
	{
		ret = -1;
		out(L"(internal error)");
	}

	out(L"\r\n");
	return ret;
}


//////////////////////////////////////////////////////////////////////////////


struct DumpSymParams
{
	STACKFRAME64* frame;
	bool locals_active;
};

// get actual address of what the symbol represents (may be relative
// to frame pointer); demarcate local/param sections; output name+value via
// dump_data_sym.
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
		addr += p->frame->AddrFrame.Offset;
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

	dump_data_sym(sym->Index, (const u8*)addr, 0);
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////


struct DumpFrameParams
{
	int left_to_skip;
};

// called by walk_stack for each stack frame
static int dump_frame_cb(STACKFRAME64* frame, void* ctx)
{
	DumpFrameParams* p = (DumpFrameParams*)ctx;

	// not the one we want yet
	if(p->left_to_skip > 0)
	{
		p->left_to_skip--;
		return 1;	// keep calling
	}

	lock();

	void* func = (void*)frame->AddrPC.Offset;
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

	DumpSymParams params = { frame, true };
	SymEnumSymbols(hProcess, 0, 0, dump_sym_cb, &params);
		// 2nd and 3rd params indicate scope set by SymSetContext
		// should be used.

	out(L"\r\n");

	unlock();
	return 1;	// keep calling
}


//////////////////////////////////////////////////////////////////////////////


// most recent <skip> stack frames will be skipped
// (we don't want to show e.g. GetThreadContext / this call)
static const wchar_t* dump_stack(uint skip, CONTEXT* thread_context = NULL)
{
	DumpFrameParams params = { (int)skip+2 };
		// skip dump_stack and walk_stack
	int err = walk_stack(dump_frame_cb, &params, thread_context);
	if(err != 0)
	{

	}
	return buf;
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

		SetDlgItemTextW(hDlg, IDC_EDIT1, buf);
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
			clipboard_set(buf);
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


// show error dialog with stack trace (must be stored in buf[])
// exits directly if 'exit' is clicked.
static int dialog(DialogType type)
{
	// we don't know if the enclosing app even has a Window.
	const HWND hParent = GetDesktopWindow();
	return (int)DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), hParent, dlgproc, (LPARAM)type);
}


// notify the user that an assertion failed; displays a
// stack trace with local variables.
// returns one of FailedAssertUserChoice or exits the program.
int debug_assert_failed(const char* file, int line, const char* expr)
{
	pos = buf;
	out(L"Assertion failed in %hs, line %d: \"%hs\"\r\n", file, line, expr);
	out(L"\r\nCall stack:\r\n\r\n");
	dump_stack(+1);	// skip this function's frame

#if defined(SCED) && !(defined(NDEBUG)||defined(TESTING))
	// ScEd keeps running while the dialog is showing, and tends to crash before
	// there's a chance to read the assert message. So, just break immediately.
	debug_break();
#endif

	return dialog(ASSERT);
}


//////////////////////////////////////////////////////////////////////////////
//
// exception handler
//
//////////////////////////////////////////////////////////////////////////////


static const wchar_t* translate(const wchar_t* text)
{
#ifdef I18N
	// make sure i18n system is (already|still) initialized.
	if(g_CurrentLocale)
	{
		// be prepared for this to fail, because translation potentially
		// involves script code and the JS context might be corrupted.
		__try
		{
			text = translate_raw(text);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
#endif

	return text;
}


// convenience wrapper on top of translate
static void translate_and_display_msg(const wchar_t* caption, const wchar_t* text)
{
	wdisplay_msg(translate(caption), translate(text));
}




// modified from http://www.codeproject.com/debug/XCrashReportPt3.asp
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
TODO
If the process is not being debugged, the function displays an Application Error message box,
depending on the current error mode. The default behavior is to display the dialog box, 
but this can be disabled by specifying SEM_NOGPFAULTERRORBOX in a call to the SetErrorMode function.
*/




static const wchar_t* exception_locus(const EXCEPTION_RECORD* er)
{
	static wchar_t locus[100];
	swprintf(locus, 18, L"0x%p", er->ExceptionAddress);
	MEMORY_BASIC_INFORMATION mbi;
	if(VirtualQuery(er->ExceptionAddress, &mbi, sizeof(mbi)))
	{
		wchar_t path[MAX_PATH];
		HMODULE hMod = (HMODULE)mbi.AllocationBase;
		if(GetModuleFileNameW(hMod, path, ARRAY_SIZE(path)))
		{
			wchar_t* filename = wcsrchr(path, '\\')+1;
			// GetModuleFileName returns full path => a '\\' exists
			swprintf(locus, ARRAY_SIZE(locus)-18, L" (%s)", filename);
		}
	}

	return locus;
}


static const wchar_t* SEH_code_to_string(DWORD code)
{
	switch(code)
	{
	case EXCEPTION_ACCESS_VIOLATION:         return L"Access violation";
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
	default:                                 return 0;
	}
}


static const wchar_t* exception_description(const EXCEPTION_RECORD* er)
{
	const ULONG_PTR* const ei = er->ExceptionInformation;
	const wchar_t* description;

	// special case for SEH access violations: display type and address.
	if(er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		const wchar_t* op = (ei[0] != 0)? L"writing" : L"reading";
		static wchar_t buf[50];
		swprintf(buf, ARRAY_SIZE(buf), L"Access violation %s 0x%08X", op, ei[1]);
		return buf;
	}

	// SEH exception.
	description = SEH_code_to_string(er->ExceptionCode);
	if(description)
		return description;

	// C++ exceptions put a pointer to the exception object
	// into ExceptionInformation[1] -- so see if it looks like
	// a PSERROR*, and use the relevant message if it is.
	__try
	{
		if (er->NumberParameters == 3)
		{
			/*/*
			PSERROR* err = (PSERROR*) ep->ExceptionRecord->ExceptionInformation[1];
			if (err->magic == 0x45725221)
			{
			int code = err->code;
			error = GetErrorString(code);
			}
			*/
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// Presumably it wasn't a PSERROR and resulted in
		// accessing invalid memory locations.
	}

	return L"unknown exception";
}


static LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS* ep)
{
	const EXCEPTION_RECORD* const er = ep->ExceptionRecord;

	// If something crashes after we've already crashed (i.e. when shutting
	// down everything), don't bother logging it, because the first crash
	// is the most important one to fix.
	static bool already_crashed = false;
	if (already_crashed)
		return EXCEPTION_EXECUTE_HANDLER;
	already_crashed = true;

	const wchar_t* description = exception_description(er);
	const wchar_t* locus       = exception_locus      (er);

	// build and display error message
	const wchar_t fmt[] = L"We are sorry, the program has encountered an error and cannot continue.\r\n"
	                      L"Please report this to http://bugs.wildfiregames.com/ and attach the crashlog.txt and crashlog.dmp files from your 'data' folder.\r\n"
						  L"Details: %s at %s.";
	wchar_t text[1000];
	swprintf(text, ARRAY_SIZE(text), translate(fmt), description, locus);
	wdisplay_msg(translate(L"Problem"), text);

	pos = buf;
	const wchar_t* stack_trace = dump_stack(+0, ep->ContextRecord);
	write_minidump(ep);
	debug_write_crashlog(description, locus, stack_trace);


	// disable memory-leak reporting to avoid a flood of warnings
	// (lots of stuff will leak since we exit abnormally).
#ifdef HAVE_DEBUGALLOC
	uint flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	_CrtSetDbgFlag(flags & ~_CRTDBG_LEAK_CHECK_DF);
#endif

	// MSDN: "This usually results in process termination".
	return EXCEPTION_EXECUTE_HANDLER;
}


// called from wdbg_init
static void set_exception_handler()
{
	SetUnhandledExceptionFilter(unhandled_exception_filter);
}
