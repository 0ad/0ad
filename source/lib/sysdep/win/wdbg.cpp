/*
 * stack walk, improved assert and exception handler
 * Copyright (c) 2002 Jan Wassenberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Contact info:
 *   Jan.Wassenberg@stud.uni-karlsruhe.de
 *   http://www.stud.uni-karlsruhe.de/~urkt/
 */

#include "precompiled.h"

#include <stdlib.h>
#include <stdio.h>

#include "lib.h"

#include "win_internal.h"
#include <dbghelp.h>

#include "wdbg.h"
#include "assert_dlg.h"


#define LOCALISED_TEXT

#ifdef LOCALISED_TEXT
#include "ps/i18n.h"
#endif // LOCALISED_TEXT




void win_debug_break()
{
	DebugBreak();
}



// need to shoehorn printf-style variable params into
// the OutputDebugString call.
// - don't want to split into multiple calls - would add newlines to output.
// - fixing Win32 _vsnprintf to return # characters that would be written,
//   as required by C99, looks difficult and unnecessary. if any other code
//   needs that, implement GNU vasprintf.
// - fixed size buffers aren't nice, but much simpler than vasprintf-style
//   allocate+expand_until_it_fits. these calls are for quick debug output,
//   not loads of data, anyway.


static const int MAX_CNT = 512;
// max output size of 1 call of (w)debug_out (including \0)



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








/*
 * voodoo programming. PDB debug info is a poorly documented mess.
 * see http://msdn.microsoft.com/msdnmag/issues/02/03/hood/default.aspx
 */


/* from DIA cvconst.h */
typedef enum
{
    btNoType    = 0,
    btVoid      = 1,
    btChar      = 2,
    btWChar     = 3,
    btInt       = 6,
    btUInt      = 7,
    btFloat     = 8,
    btBCD       = 9,
    btBool      = 10,
    btLong      = 13,
    btULong     = 14,
    btCurrency  = 25,
    btDate      = 26,
    btVariant   = 27,
    btComplex   = 28,
    btBit       = 29,
    btBSTR      = 30,
    btHresult   = 31
}
BasicType;

static void DumpMiniDump(HANDLE hFile, PEXCEPTION_POINTERS excpInfo);

// function pointers
//
// we need to link against dbghelp dynamically - some systems may not
// have v5.1. (if <= 5.0, type information is unavailable).
//
// don't delay-load: requires the .lib file, which may not be present
// on all compilers.
//
// can't skip the dbghelp.h function prototypes, so we need to
// name the function pointers differently (prepend _). Grr.
//
#define FUNC(ret, name, params) static ret (WINAPI *_##name) params;
#include "dbghelp_funcs.h"
#undef FUNC


static HANDLE hProcess;
static uintptr_t mod_base;

const int bufsize = 64000;
static wchar_t buf[bufsize];	/* buffer for stack trace */
static wchar_t* pos;			/* current pos in buf */


static void out(wchar_t* fmt, ...)
{
	// Don't overflow the buffer (and abort if we're about to)
	if (pos-buf+1000 > bufsize)
		throw;

	va_list args;
	va_start(args, fmt);
	pos += vswprintf(pos, 1000, fmt, args);
	va_end(args);
}


static void dbg_cleanup(void)
{
	_SymCleanup(hProcess);
}

static void dbg_init()
{
	/* already initialized */
	if(hProcess)
		return;

	HMODULE hdbghelp = LoadLibrary("dbghelp.dll");

// import functions
#define FUNC(ret, name, params) *(void**)&_##name = (void*)GetProcAddress(hdbghelp, #name);
#include "dbghelp_funcs.h"
#undef FUNC

	hProcess = GetCurrentProcess();

	_SymSetOptions(SYMOPT_DEFERRED_LOADS);
	_SymInitialize(hProcess, 0, TRUE);

	atexit(dbg_cleanup);
}


/*
 * guess if addr points to a string
 * algorithm:
 *   scan string; count # text chars vs. garbage
 */
static bool is_string_ptr(u64 addr)
{
	/* early out for x86 */
#ifdef _WIN32
	if(addr < 0x10000 || addr > 0x80000000)
		return false;
#endif

	const char* str = (const char*)addr;

	__try
	{
		int score = 0;
		for(;;)
		{
			const int c = *str++ & 0xff;	/* signed char => promoted */
			if(isalnum(c))
				score += 1;
			else if(!c)
				return (score > 0);			/* end of string reached */
			else if(!isprint(c))
				score -= 5;

			/* give up */
			if(score <= -10)
				return false;
		}
	}
	__except(1)
	{
		return false;
	}
}


/*
 * output the value of 'basic' types: string, (array of) float, int
 *
 * not reliably supported, due to broken debug info / bad documentation:
 * - multidimensional arrays (e.g. int[2][2] => len 16, elements 2)
 * - arrays of character arrays (would have rely on is_string_ptr)
 * - nested structures (invalid base type index)
 */
static void print_var(u32 type_idx, uint level, u64 addr)
{
	// successful with index we were given?
	BasicType type = btNoType;
	u32 real_typeid = 0;
	if(!_SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_BASETYPE, &type))
	{
		// no; try to get the "real" index first
		BOOL ok = _SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_TYPEID, &real_typeid);
		if(ok && real_typeid != type_idx)
			_SymGetTypeInfo(hProcess, mod_base, real_typeid, TI_GET_BASETYPE, &type);
	}

	u64 len;
	_SymGetTypeInfo(hProcess, mod_base, level? real_typeid : type_idx, TI_GET_LENGTH, &len);

	u32 elements;
	_SymGetTypeInfo(hProcess, mod_base, level? real_typeid : type_idx, TI_GET_COUNT, &elements);

	wchar_t* fmt = L"%I64X";	/* default: 64 bit integer */

	out(L"= ");

	/*
	 * single string (character array)
	 * len == elements => bool array | character array (probably string)
	 */
	if(len == elements && is_string_ptr(addr))
	{
		out(L"\"%hs\"\r\n", (char*)addr);
		return;
	}

	/* array */
	if(elements)
	{
		len /= elements;
		out(L"{ ");
	}

	u32 totalelements = elements;

next_element:

	/* zero extend to 64 bit (little endian) */
	u64 data = 0;
	for(u64 i = 0; i < MIN(len,8); i++)
		data |= (u64)(*(u8*)(addr+i)) << (i*8);

	/* float / double */
	if(type == btFloat)
	{
		/* convert floats to double (=> single printf call) */
		if(len == 4)
		{
			double double_data = (double)(*(float*)addr);
			data = *(u64*)&double_data;
		}
		fmt = L"%lf";
	}
	/* string (char*) */
	else if(len == sizeof(void*) && is_string_ptr(data))
	{
		fmt = L"\"%hs\"";
	}

	out(fmt, data);

	/* ascii char */
	if(data < 0x100 && isprint((int)data))
		out(L" ('%hc')", (char)data);

	if(elements)
	{
		addr += len;	/* add stride */
		elements--;

		/* don't display too many elements of huge arrays */
		if (totalelements-elements > 32)
			out(L" ... }");
		
		else if(elements == 0)
			out(L" }");

		else
		{
			out(L", ");
			goto next_element;
		}
	}

	out(L"\r\n");
}


/* print sym name; if not a basic type, recurse for each child */
static void dump_sym(u32 type_idx, uint level, u64 addr)
{
	/* print type name / member name */
	WCHAR* type_name;
	if(_SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_SYMNAME, &type_name))
	{
		/* ignore things that look like member functions, because
		   there's no point in outputting them */
		if (wcsstr(type_name, L"::"))
		{
			LocalFree(type_name);
			return;
		}

		/* indent */
		for(uint i = 0; i < level+2; i++)
			out(L"  ");

		out(L"%ls ", type_name);

		/* exclude some specific variable types, because
		   they just fill the stack trace with rubbish */
		if (!wcscmp(type_name, L"JSType")
		 || !wcscmp(type_name, L"JSVersion")
			)
		{
			LocalFree(type_name);
			out(L"\r\n");
			return;
		}

		LocalFree(type_name);
	}

	u32 num_children = 0;

	/* built-in type? yes - show its value, terminate recursion */
	if (! _SymGetTypeInfo(hProcess, mod_base, type_idx, TI_GET_CHILDRENCOUNT, &num_children)
		|| num_children == 0)
	{
		print_var(type_idx, level, addr);
		return;
	}

	// bogus
	if(num_children > 50)
		return;

	/* get children's type indices */
	char child_buf[sizeof(TI_FINDCHILDREN_PARAMS) + 256*4];
	TI_FINDCHILDREN_PARAMS* children = (TI_FINDCHILDREN_PARAMS*)child_buf;
	children->Count = num_children;
	children->Start = 0;
	_SymGetTypeInfo(hProcess, mod_base, type_idx, TI_FINDCHILDREN, children);

    out(L"\r\n");

	/* recurse for each child */
	for(uint child_idx = 0; child_idx < num_children; child_idx++)
	{
		u32 child_id = children->ChildId[child_idx];

		u32 member_ofs;
		_SymGetTypeInfo(hProcess, mod_base, child_id, TI_GET_OFFSET, &member_ofs);

		dump_sym(child_id, level+1, addr+member_ofs);
    }
}


static enum
{
	NONE,
	PARAMS,
	LOCALS
}
var_type;


#define SYM_FLAG(flag) (sym->Flags & (IMAGEHLP_SYMBOL_INFO_##flag))


/* called for each local symbol */
static BOOL CALLBACK sym_callback(SYMBOL_INFO* sym, ULONG /* SymbolSize */, void* ctx)
{
	STACKFRAME64* frame = (STACKFRAME64*)ctx;

	mod_base = (uintptr_t)sym->ModBase;

	__try
	{
		if(SYM_FLAG(PARAMETER))
		{
			if(var_type != PARAMS)
			{
				var_type = PARAMS;
				out(L"  params:\r\n");
			}
		}
		else
			if(var_type != LOCALS)
			{
				var_type = LOCALS;
				out(L"  locals:\r\n");
			}

		out(L"    %hs ", sym->Name);

		u64 addr = sym->Address;	/* -> var's contents */
		if(SYM_FLAG(REGRELATIVE))
			addr += frame->AddrFrame.Offset;
		else if(SYM_FLAG(REGISTER))
			return 0;

		dump_sym(sym->TypeIndex, 0, addr);
	}
    __except(1)
    {
		out(L"\r\nError reading symbol %hs!\r\n", sym->Name);
    }

	return 1;
}


/*
 * most recent <skip> stack frames will be skipped
 * (we don't want to show e.g. GetThreadContext / this call)
 */
static void walk_stack(int skip, wchar_t* out_buf, CONTEXT* override_context = NULL)
{
	dbg_init();

	bool type_avail = _SymFromAddr != 0;

	pos = out_buf;
	out(L"\r\nCall stack:\r\n\r\n");

	if(!type_avail)
		out(L"warning: older dbghelp.dll loaded; no type information available.\r\n");

	HANDLE hThread = GetCurrentThread();

	CONTEXT context;
	if(override_context)
	{
		memcpy(&context, override_context, sizeof(CONTEXT));
	}
	else
	{
		memset(&context, 0, sizeof(context));
		context.ContextFlags = CONTEXT_FULL;
		GetThreadContext(hThread, &context);
	}

	STACKFRAME64 frame;
	memset(&frame, 0, sizeof(frame));

#ifdef _M_IX86
	/* x86 only: init STACKFRAME for first StackWalk call (undocumented) */
	frame.AddrPC.Offset     = context.Eip;
	frame.AddrPC.Mode       = AddrModeFlat;
	frame.AddrStack.Offset  = context.Esp;
	frame.AddrStack.Mode    = AddrModeFlat;
	frame.AddrFrame.Offset  = context.Ebp;
	frame.AddrFrame.Mode    = AddrModeFlat;
#endif

	u64 base = _SymGetModuleBase64(hProcess, (u64)&walk_stack);
	IMAGE_NT_HEADERS* header = _ImageNtHeader((void*)base);
	u32 machine = header->FileHeader.Machine;

	char sym_buf[sizeof(SYMBOL_INFO) + 1024];
	memset(sym_buf, 0, sizeof(sym_buf));
	SYMBOL_INFO* sym = (SYMBOL_INFO*)sym_buf;
	sym->SizeOfStruct = sizeof(SYMBOL_INFO);
	sym->MaxNameLen = 1024;

	IMAGEHLP_LINE64 line = { sizeof(IMAGEHLP_LINE64) };
	IMAGEHLP_STACK_FRAME imghlp_frame;

	for(;;)
    {
		if(!_StackWalk64(machine, hProcess, hThread, &frame, &context, 0, _SymFunctionTableAccess64, _SymGetModuleBase64, 0))
            break;

		if(skip-- > 0)
			continue;

		u64 pc = frame.AddrPC.Offset;

		if(!type_avail)
		{
			out(L"%I64X\r\n", pc);
			continue;
		}

		/* function name / address */
		u64 sym_disp;
        if(type_avail && _SymFromAddr(hProcess, pc, &sym_disp, sym))
			out(L"%hs", sym->Name);
		else
			out(L"%I64X", pc);

		/* source file + line number */
		DWORD line_disp;
        if(_SymGetLineFromAddr64(hProcess, pc, &line_disp, &line))
			out(L" (%hs, line %lu)", line.FileName, line.LineNumber);

		out(L"\r\n");

		/* params + vars */
		var_type = NONE;

		imghlp_frame.InstructionOffset = pc;
		_SymSetContext(hProcess, &imghlp_frame, 0);
		_SymEnumSymbols(hProcess, 0, 0, sym_callback, &frame);

		out(L"\r\n");
    }
}


/*---------------------------------------------------------------------------*/


typedef enum
{
	ASSERT,
	EXCEPTION
}
DLG_TYPE;


static int CALLBACK dlgproc(HWND hDlg, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
	{
		DLG_TYPE type = (DLG_TYPE)lParam;

		/* disable inappropriate buttons */
		if(type != ASSERT)
		{
			HWND h;
			h = GetDlgItem(hDlg, IDC_CONTINUE);
			EnableWindow(h, 0);
			h = GetDlgItem(hDlg, IDC_SUPPRESS);
			EnableWindow(h, 0);
			h = GetDlgItem(hDlg, IDC_BREAK);
			EnableWindow(h, 0);
		}

		SetDlgItemTextW(hDlg, IDC_EDIT1, buf);
		return 1;	/* allow focus to be set */
	}

	case WM_SYSCOMMAND:
		if(wParam == SC_CLOSE)
			EndDialog(hDlg, 0);
		return 0;	/* allows dragging; don't understand why. */

	case WM_COMMAND:
		switch(wParam)
		{
		case IDC_COPY:
			clipboard_set(buf);
			break;

		case IDC_CONTINUE:	/* 2000 */
		case IDC_SUPPRESS:
		case IDC_BREAK:		/* 2002 */
			EndDialog(hDlg, wParam-2000);
			break;

		case IDC_EXIT:
			exit(0);
		}
		return 1;
	}

	return 0;	/* do default processing for msg */
}




/*
 * show error dialog, with stack trace (must be stored in buf[])
 * exits directly if 'exit' is clicked.
 *
 * return values:
 *   0 - continue
 *   1 - suppress
 *   2 - break
 */
static int dialog(DLG_TYPE type)
{
	return (int)DialogBoxParam(0, MAKEINTRESOURCE(IDD_DIALOG1), 0, dlgproc, (LPARAM)type);
}


/*---------------------------------------------------------------------------*/


static PTOP_LEVEL_EXCEPTION_FILTER prev_except_filter;

static long CALLBACK except_filter(EXCEPTION_POINTERS* except)
{
	PEXCEPTION_RECORD except_record = except->ExceptionRecord;
	uintptr_t addr = (uintptr_t)except_record->ExceptionAddress;

	DWORD code = except_record->ExceptionCode;
	char* except_str = "(unknown)";
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
	}
#undef X

	MEMORY_BASIC_INFORMATION mbi;
	char module_buf[100];
	char* module = "???";
	uintptr_t base = 0;

	if(VirtualQuery((void*)addr, &mbi, sizeof(mbi)))
	{
		base = (uintptr_t)mbi.AllocationBase;
		if(GetModuleFileName((HMODULE)base, module_buf, sizeof(module_buf)))
			module = strrchr(module_buf, '\\')+1;
			// GetModuleFileName returns fully qualified path =>
			// trailing '\\' exists
	}


	int pos = swprintf(buf, 1000, L"Exception %hs at %hs!%08lX\r\n", except_str, module, addr-base);
	walk_stack(0, buf+pos, except->ContextRecord);

	dialog(EXCEPTION);

	if(prev_except_filter)
		return prev_except_filter(except);

	return EXCEPTION_CONTINUE_EXECUTION;
}


static void set_exception_handler()
{
	prev_except_filter = SetUnhandledExceptionFilter(except_filter);
}



#ifdef LOCALISED_TEXT

// Split this into a separate function because destructors and __try don't mix
void i18n_display_fatal_msg(const wchar_t* errortext) {
	CStrW title = translate(L"Pyrogenesis Failure");
	CStrW message = translate(L"A fatal error has occurred: $msg. Please report this to http://bugs.wildfiregames.com/ and attach the crashlog.txt and crashlog.dmp files from your 'data' folder.") << I18n::Raw(errortext);
	wdisplay_msg(title.c_str(), message.c_str());
}

#endif // LOCALISED_TEXT

// PT: Alternate version of the exception handler, which makes
// the crash log more useful, and takes the responsibility of
// suiciding away from main.cpp.

void abort_timer(); // from wtime.cpp

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

	const wchar_t* error = NULL;

	// C++ exceptions put a pointer to the exception object
	// into ExceptionInformation[1] -- so see if it looks like
	// a PSERROR*, and use the relevant message if it is.
	__try
	{
		if (ep->ExceptionRecord->NumberParameters == 3)
		{
			PSERROR* err = (PSERROR*) ep->ExceptionRecord->ExceptionInformation[1];
			if (err->magic == 0x45725221)
			{
				int code = err->code;
				error = GetErrorString(code);
			}
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
		debug_write_crashlog("crashlog.txt", errortext, ep->ContextRecord);
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




static void init()
{
	ONCE(dbg_init());
}


int debug_assert_failed(const char* file, int line, const char* expr)
{
	init();

	int pos = swprintf(buf, 1000, L"Assertion failed in %hs, line %d: %hs\r\n", file, line, expr);
	walk_stack(2, buf+pos);

	return dialog(ASSERT);
}


// main.cpp will have an exception handler, but this covers low-level init
// (before main is called)
static int wdbg_init()
{
	set_exception_handler();
	return 0;
}


#pragma data_seg(".LIB$WIB")	// first
WIN_REGISTER_FUNC(wdbg_init);
#pragma data_seg()


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
		if (!_MiniDumpWriteDump(
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
		fwprintf(out, L"%hs", &buffer);
	}
}

// Imported from sysdep.cpp
extern wchar_t MicroBuffer[];
extern int MicroBuffer_off;

int debug_write_crashlog(const char* file, wchar_t* header, void* context)
{
	init();

	int len = swprintf(buf, 1000, L"Undefined state reached.\r\n");

	__try
	{
		if (context)
			walk_stack(0, buf+len, (CONTEXT*)context);
		else
			walk_stack(2, buf+len);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// Probably caught a buffer overflow
	}

	FILE* f = fopen(file, "wb");
	int BOM = 0xFEFF;
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