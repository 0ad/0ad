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

#include "wdbg.h"
#include "assert_dlg.h"


#ifdef _MSC_VER
#pragma comment(lib, "dbghelp.lib")
#endif


// automatic module init (before main) and shutdown (before termination)
#pragma data_seg(".LIB$WCC")
WIN_REGISTER_FUNC(wdbg_init);
#pragma data_seg(".LIB$WTB")
WIN_REGISTER_FUNC(wdbg_shutdown);
#pragma data_seg()


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

// max output size of 1 call of (w)debug_out (including \0)
static const int MAX_CNT = 512;


void debug_out(const char* fmt, ...)
{
	char buf[MAX_CNT];
	buf[MAX_CNT-1] = '\0';

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, MAX_CNT-1, fmt, ap);
	va_end(ap);

	OutputDebugString(buf);
}


void wdebug_out(const wchar_t* fmt, ...)
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
// dbghelp support routines for walking the stack
//
//////////////////////////////////////////////////////////////////////////////

// dbghelp isn't thread-safe. lock is taken by walk_stack,
// debug_resolve_symbol, and while dumping a stack frame (dump_frame_cb).
static void lock()
{
	win_lock(DBGHELP_CS);
}

static void unlock()
{
	win_unlock(DBGHELP_CS);
}

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
	// StackWalk call in our loop. there is no portable way to do this,
	// since CONTEXT is platform-specific. if the caller passed in a thread
	// context (e.g. if calling from an exception handler), we use that;
	// otherwise, determine the current PC / frame pointer ourselves.
	// GetThreadContext is documented not to work if the current thread
	// is running, but that seems to be current practice. Regardless, we
	// avoid using it, since simple asm code is safer.
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
	int ret = -1;

	lock();

	{
	const DWORD64 addr = (DWORD64)ptr_of_interest;

	// get symbol name
	SYMBOL_INFO_PACKAGE sp;
	SYMBOL_INFO* sym = &sp.si;
	sym->SizeOfStruct = sizeof(sp.si);
	sym->MaxNameLen = MAX_SYM_NAME;
	if(!SymFromAddr(hProcess, addr, 0, sym))
		goto fail;
	sprintf(sym_name, "%s", sym->Name);

	// get source file + line number
	IMAGEHLP_LINE64 line_info = { sizeof(IMAGEHLP_LINE64) };
	DWORD displacement; // unused but required by SymGetLineFromAddr64!
	if(!SymGetLineFromAddr64(hProcess, addr, &displacement, &line_info))
		goto fail;
	sprintf(file, "%s", line_info.FileName);
	*line = line_info.LineNumber;

	ret = 0;
	}

fail:
	unlock();
	return ret;
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
	// completely bogus on IA32; save ourselves the segfault (slow).
#ifdef _WIN32
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
			debug_warn("dump_udt: warning: TI_GET_OFFSET query failed");

		int ret = dump_data_sym(child_data_idx, p+ofs, level+1);

		// remember first error
		if(err == 0)
			err = ret;
	}

	return err;
}


//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

// given a data symbol's type identifier, output its type name (if
// applicable), determine what kind of variable it describes, and
// call the appropriate dump_* routine.
// lock is held.
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
		debug_out("Unknown tag: %d\n", type_tag);
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
	SYMBOL_INFO_PACKAGEW sp;
	SYMBOL_INFOW* sym = &sp.si;
	sym->SizeOfStruct = sizeof(sp.si);
	sym->MaxNameLen = MAX_SYM_NAME;
	if(!SymFromIndexW(hProcess, mod_base, data_idx, sym))
		return -1;

	if(sym->Tag != SymTagData)
	{
		assert(sym->Tag == SymTagData);
		return -1;
	}


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
			out(L"params:\r\n");
			p->locals_active = false;
		}
	}
	else if(sym->Flags & SYMF_LOCAL)
	{
		if(!p->locals_active)
		{
			out(L"locals:\r\n ");
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
static void dump_stack(uint skip, CONTEXT* thread_context = NULL)
{
	out(L"\r\nCall stack:\r\n\r\n");

	DumpFrameParams params = { (int)skip+2 };
		// skip dump_stack and walk_stack
	walk_stack(dump_frame_cb, &params, thread_context);
}


//////////////////////////////////////////////////////////////////////////////
//
// "program error" dialog with stack trace (triggered by assert and exception)
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
static POINTS prev_dlg_client_size;

const int ANCHOR_LEFT   = 0x01;
const int ANCHOR_RIGHT  = 0x02;
const int ANCHOR_TOP    = 0x04;
const int ANCHOR_BOTTOM = 0x08;
const int ANCHOR_ALL    = 0x0f;

static void resize_control(HWND hDlg, int dlg_item, int dx,int dy, int anchors)
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


static void onSize(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	// 'minimize' was clicked. we need to ignore this, otherwise
	// dx/dy would reduce some control positions to less than 0.
	// since Windows clips them, we wouldn't later be able to
	// reconstruct the previous values when 'restoring'.
	if(wParam == SIZE_MINIMIZED)
		return;

	// first call for this dialog instance. WM_MOVE hasn't been sent yet,
	// so dlg_client_origin are invalid => must not call resize_control().
	// we need to set prev_dlg_client_size for the next call before exiting.
	bool first_call = (prev_dlg_client_size.y == 0);

	POINTS dlg_client_size = MAKEPOINTS(lParam);
	int dx = dlg_client_size.x - prev_dlg_client_size.x;
	int dy = dlg_client_size.y - prev_dlg_client_size.y;
	prev_dlg_client_size = dlg_client_size;

	if(first_call)
		return;

	resize_control(hDlg, IDC_CONTINUE, dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	resize_control(hDlg, IDC_SUPPRESS, dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	resize_control(hDlg, IDC_BREAK   , dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	resize_control(hDlg, IDC_EXIT    , dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	resize_control(hDlg, IDC_COPY    , dx,dy, ANCHOR_RIGHT|ANCHOR_BOTTOM);
	resize_control(hDlg, IDC_EDIT1   , dx,dy, ANCHOR_ALL);
}


static int CALLBACK dlgproc(HWND hDlg, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
		// need to reset for new instance of dialog
		dlg_client_origin.x = dlg_client_origin.y = 0;
		prev_dlg_client_size.x = prev_dlg_client_size.y = 0;
			
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

		case IDC_CONTINUE:	// 2000
		case IDC_SUPPRESS:
		case IDC_BREAK:		// 2002
			EndDialog(hDlg, wParam-2000);
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
		onSize(hDlg, wParam, lParam);
		break;

	default:
		break;
	}

	// we didn't process the message; caller will perform default action.
	return FALSE;
}


// show error dialog, with stack trace (must be stored in buf[])
// exits directly if 'exit' is clicked.
//
// return values:
//   0 - continue
//   1 - suppress
//   2 - break

static int dialog(DialogType type)
{
	// we don't know if the enclosing app even has a Window.
	const HWND hParent = GetDesktopWindow();
	return (int)DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), hParent, dlgproc, (LPARAM)type);
}


int debug_assert_failed(const char* file, int line, const char* expr)
{
	pos = buf;
	out(L"Assertion failed in %hs, line %d: \"%hs\"\r\n", file, line, expr);
	dump_stack(1);	// skip this function's frame

	return dialog(ASSERT);
}


//////////////////////////////////////////////////////////////////////////////
//
// exception handler
//
//////////////////////////////////////////////////////////////////////////////


static PTOP_LEVEL_EXCEPTION_FILTER prev_except_filter;

static long CALLBACK except_filter(EXCEPTION_POINTERS* except)
{
	PEXCEPTION_RECORD except_record = except->ExceptionRecord;
	uintptr_t addr = (uintptr_t)except_record->ExceptionAddress;

	DWORD code = except_record->ExceptionCode;
	const char* except_str;
#define X(e) case EXCEPTION_##e: except_str = #e; break;
	switch(code)
	{
		X(ACCESS_VIOLATION)
		X(DATATYPE_MISALIGNMENT)
		X(BREAKPOINT)
		X(SINGLE_STEP)
		X(ARRAY_BOUNDS_EXCEEDED)
		X(FLT_DENORMAL_OPERAND)
		X(FLT_DIVIDE_BY_ZERO)
		X(FLT_INEXACT_RESULT)
		X(FLT_INVALID_OPERATION)
		X(FLT_OVERFLOW)
		X(FLT_STACK_CHECK)
		X(FLT_UNDERFLOW)
		X(INT_DIVIDE_BY_ZERO)
		X(INT_OVERFLOW)
		X(PRIV_INSTRUCTION)
		X(IN_PAGE_ERROR)
		X(ILLEGAL_INSTRUCTION)
		X(NONCONTINUABLE_EXCEPTION)
		X(STACK_OVERFLOW)
		X(INVALID_DISPOSITION)
		X(GUARD_PAGE)
		X(INVALID_HANDLE)
	default:
		except_str = "(unknown)";
	}
#undef X

	MEMORY_BASIC_INFORMATION mbi;
	char module_buf[100];
	const char* module = "???";
	uintptr_t base = 0;

	if(VirtualQuery((void*)addr, &mbi, sizeof(mbi)))
	{
		base = (uintptr_t)mbi.AllocationBase;
		if(GetModuleFileName((HMODULE)base, module_buf, sizeof(module_buf)))
			module = strrchr(module_buf, '\\')+1;
			// GetModuleFileName returns fully qualified path =>
			// trailing '\\' exists
	}


	pos = buf;
	out(L"Exception %hs at %hs!%08lX\r\n", except_str, module, addr-base);
	dump_stack(0, except->ContextRecord);

	dialog(EXCEPTION);

	if(prev_except_filter)
		return prev_except_filter(except);

	return EXCEPTION_CONTINUE_EXECUTION;
}


static void set_exception_handler()
{
	prev_except_filter = SetUnhandledExceptionFilter(except_filter);
}




#ifndef NO_0AD_EXCEPTION








#ifdef LOCALISED_TEXT

// Split this into a separate function because destructors and __try don't mix
void i18n_display_fatal_msg(const wchar_t* errortext) {
	CStrW title = translate(L"Pyrogenesis Failure");
	CStrW message = translate(L"A fatal error has occurred: $msg. Please report this to http://bugs.wildfiregames.com/ and attach the crashlog.txt and crashlog.dmp files from your 'data' folder.") << I18n::Raw(errortext);
	wdisplay_msg(title.c_str(), message.c_str());
}

#endif // LOCALISED_TEXT






// from http://www.codeproject.com/debug/XCrashReportPt3.asp
static void DumpMiniDump(HANDLE hFile, PEXCEPTION_POINTERS excpInfo)
{
/*
	if (excpInfo == NULL) 
	{
		// Generate exception to get proper context in dump
		__try 
		{
			OutputDebugString("RaiseException for MinidumpWriteDump\n");
			RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
		} 
		__except(DumpMiniDump(hFile, GetExceptionInformation()), EXCEPTION_CONTINUE_EXECUTION) 
		{
		}
	}
	else
	{*/
		MINIDUMP_EXCEPTION_INFORMATION eInfo;
		eInfo.ThreadId = GetCurrentThreadId();
		eInfo.ExceptionPointers = excpInfo;
		eInfo.ClientPointers = FALSE;

		// TODO: Store the crashlog.txt inside the UserStreamParam
		// so that people only have to send us one file?

		// note:  MiniDumpWithIndirectlyReferencedMemory does not work on Win98
		if (!MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hFile,
			MiniDumpNormal,
			excpInfo ? &eInfo : NULL,
			NULL,
			NULL))
		{
			throw; // fail noisily
		}
//	}
}

static void cat_atow(FILE* in, FILE* out)
{
	const int bufsize = 1024;
	char buffer[bufsize+1]; // bufsize+1 so there's space for a \0 at the end
	while (!feof(in))
	{
		int r = (int)fread(buffer, 1, bufsize, in);
		if (!r) break;
		buffer[r] = 0; // ensure proper NULLness
		fwprintf(out, L"%hs", buffer);
	}
}

// Imported from sysdep.cpp
extern wchar_t MicroBuffer[];
extern size_t MicroBuffer_off;

static int write_crashlog(const char* file, const wchar_t* header, CONTEXT* context)
{
	pos = buf;
	out(L"Unhandled exception.\r\n");

	dump_stack(0, context);

	FILE* f = fopen(file, "wb");
	u16 BOM = 0xFEFF;
	fwrite(&BOM, 2, 1, f);
	if (header)
	{
		fwrite(header, wcslen(header), sizeof(wchar_t), f);
		fwrite(L"\r\n\r\n", 4, sizeof(wchar_t), f);
	}
	fwrite(buf, pos-buf, 2, f);

	const wchar_t* divider = L"\r\n\r\n====================================\r\n\r\n";

	fwrite(divider, wcslen(divider), sizeof(wchar_t), f);

	// HACK: Insert the contents from a couple of other log files
	// and one memory log. These really ought to be integrated better.

	// Copy the contents of ../logs/systeminfo.txt
	FILE* in;
	in = fopen("../logs/system_info.txt", "rb");
	if (in)
	{
		fwprintf(f, L"System info:\r\n\r\n");
		cat_atow(in, f);
		fclose(in);
	}

	fwrite(divider, wcslen(divider), sizeof(wchar_t), f);

	// Copy the contents of ../logs/mainlog.html
	in = fopen("../logs/mainlog.html", "rb");
	if (in)
	{
		fwprintf(f, L"Main log:\r\n\r\n");
		cat_atow(in, f);
		fclose(in);
	}

	fwrite(divider, wcslen(divider), sizeof(wchar_t), f);

	fwprintf(f, L"Last known activity:\r\n\r\n");
	fwrite(MicroBuffer, MicroBuffer_off, sizeof(wchar_t), f);
	fclose(f);

	return 0;
}


// PT: Alternate version of the exception handler, which makes
// the crash log more useful, and takes the responsibility of
// suiciding away from main.cpp.

int debug_main_exception_filter(unsigned int UNUSEDPARAM(code), PEXCEPTION_POINTERS ep)
{
	// If something crashes after we've already crashed (i.e. when shutting
	// down everything), don't bother logging it, because the first crash
	// is the most important one to fix.
	static bool already_crashed = false;
	if (already_crashed)
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}
	already_crashed = true;

/*
	// The timer thread sometimes dies from EXCEPTION_PRIV_INSTRUCTION
	// when debugging this exception handler code (which gets quite
	// annoying), so kill it before it gets a chance.
	__try
	{
		abort_timer();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
*/

	const wchar_t* error = NULL;

	// C++ exceptions put a pointer to the exception object
	// into ExceptionInformation[1] -- so see if it looks like
	// a PSERROR*, and use the relevant message if it is.
	__try
	{
		if (ep->ExceptionRecord->NumberParameters == 3)
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
		error = NULL;
	}

	// Try getting nice names for other types of error:
	if (! error)
	{
		switch (ep->ExceptionRecord->ExceptionCode)
		{
		case EXCEPTION_ACCESS_VIOLATION:		error = L"Access violation"; break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:	error = L"Datatype misalignment"; break;
		case EXCEPTION_BREAKPOINT:				error = L"Breakpoint"; break;
		case EXCEPTION_SINGLE_STEP:				error = L"Single step"; break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:	error = L"Array bounds exceeded"; break;
		case EXCEPTION_FLT_DENORMAL_OPERAND:	error = L"Float denormal operand"; break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:		error = L"Float divide by zero"; break;
		case EXCEPTION_FLT_INEXACT_RESULT:		error = L"Float inexact result"; break;
		case EXCEPTION_FLT_INVALID_OPERATION:	error = L"Float invalid operation"; break;
		case EXCEPTION_FLT_OVERFLOW:			error = L"Float overflow"; break;
		case EXCEPTION_FLT_STACK_CHECK:			error = L"Float stack check"; break;
		case EXCEPTION_FLT_UNDERFLOW:			error = L"Float underflow"; break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:		error = L"Integer divide by zero"; break;
		case EXCEPTION_INT_OVERFLOW:			error = L"Integer overflow"; break;
		case EXCEPTION_PRIV_INSTRUCTION:		error = L"Privileged instruction"; break;
		case EXCEPTION_IN_PAGE_ERROR:			error = L"In page error"; break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:		error = L"Illegal instruction"; break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:error = L"Noncontinuable exception"; break;
		case EXCEPTION_STACK_OVERFLOW:			error = L"Stack overflow"; break;
		case EXCEPTION_INVALID_DISPOSITION:		error = L"Invalid disposition"; break;
		case EXCEPTION_GUARD_PAGE:				error = L"Guard page"; break;
		case EXCEPTION_INVALID_HANDLE:			error = L"Invalid handle"; break;
		default: error = L"[unknown exception]";
		}
	}

	wchar_t* errortext = (wchar_t*) error;

	// Handle access violations specially, because we can see
	// whether and where they were reading/writing.
	if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		errortext = new wchar_t[256];
		if (ep->ExceptionRecord->ExceptionInformation[0])
			swprintf(errortext, 256, L"Access violation writing 0x%08X", ep->ExceptionRecord->ExceptionInformation[1]);
		else
			swprintf(errortext, 256, L"Access violation reading 0x%08X", ep->ExceptionRecord->ExceptionInformation[1]);
	}

	bool localised_successfully = false;
#ifdef LOCALISED_TEXT

	// In case this is called before/after the i18n system is
	// alive, make sure it's actually a valid pointer
	if (g_CurrentLocale)
	{
		__try
		{
			i18n_display_fatal_msg(errortext);
			localised_successfully = true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
#endif // LOCALISED_TEXT
	if (!localised_successfully)
	{
		wchar_t message[1024];
		swprintf(message, 1024, L"A fatal error has occurred: %ls.\nPlease report this to http://bugs.wildfiregames.com/ and attach the crashlog.txt and crashlog.dmp files from your 'data' folder.", errortext);
		message[1023] = 0;

		wdisplay_msg(L"Pyrogenesis failure", message);
	}


	__try
	{
		write_crashlog("crashlog.txt", errortext, ep->ContextRecord);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		wdisplay_msg(L"Pyrogenesis failure", L"Error generating crash log.");
	}

	__try
	{
		HANDLE hFile = CreateFile("crashlog.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE) throw;
		DumpMiniDump(hFile, ep);
		CloseHandle(hFile);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		wdisplay_msg(L"Pyrogenesis failure", L"Error generating crash dump.");
	}

	// Disable memory-leak reporting, because it's going to
	// leak like a bucket with a missing bottom when it crashes.
#ifdef HAVE_DEBUGALLOC
	uint flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	_CrtSetDbgFlag(flags & ~_CRTDBG_LEAK_CHECK_DF);
#endif

	exit(EXIT_FAILURE);
}

#endif	// #ifndef NO_0AD_EXCEPTION

