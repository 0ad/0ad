/* Copyright (C) 2010 Wildfire Games.
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
 * Win32 debug support code.
 */

#include "precompiled.h"
#include "lib/debug.h"

#include "lib/bits.h"
#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/wutil.h"


// return 1 if the pointer appears to be totally bogus, otherwise 0.
// this check is not authoritative (the pointer may be "valid" but incorrect)
// but can be used to filter out obviously wrong values in a portable manner.
int debug_IsPointerBogus(const void* p)
{
	if(p < (void*)0x10000)
		return true;
#if ARCH_AMD64
	if(p == (const void*)(uintptr_t)0xCCCCCCCCCCCCCCCCull)
		return true;
#elif ARCH_IA32
	if(p == (const void*)(uintptr_t)0xCCCCCCCCul)
		return true;
#endif

	// notes:
	// - we don't check alignment because nothing can be assumed about a
	//   string pointer and we mustn't reject any actually valid pointers.
	// - nor do we bother checking the address against known stack/heap areas
	//   because that doesn't cover everything (e.g. DLLs, VirtualAlloc).
	// - cannot use IsBadReadPtr because it accesses the mem
	//   (false alarm for reserved address space).

	return false;
}


bool debug_IsCodePointer(void* p)
{
	uintptr_t addr = (uintptr_t)p;
	// totally invalid pointer
	if(debug_IsPointerBogus(p))
		return false;
	// comes before load address
	static const HMODULE base = GetModuleHandle(0);
	if(addr < (uintptr_t)base)
		return false;

	return true;
}


bool debug_IsStackPointer(void* p)
{
	uintptr_t addr = (uintptr_t)p;
	// totally invalid pointer
	if(debug_IsPointerBogus(p))
		return false;
	// not aligned
	if(addr % sizeof(void*))
		return false;
	// out of bounds (note: IA-32 stack grows downwards)
	NT_TIB* tib = (NT_TIB*)NtCurrentTeb();
	if(!(tib->StackLimit < p && p < tib->StackBase))
		return false;

	return true;
}


void debug_puts(const char* text)
{
	OutputDebugStringW(wstring_from_utf8(text).c_str());
}


void wdbg_printf(const wchar_t* fmt, ...)
{
	wchar_t buf[1024+1];	// wvsprintfW will truncate to this size
	va_list ap;
	va_start(ap, fmt);
	wvsprintfW(buf, fmt, ap);	// (return value doesn't indicate whether truncation occurred)
	va_end(ap);

	OutputDebugStringW(buf);
}


// inform the debugger of the current thread's description, which it then
// displays instead of just the thread handle.
//
// see "Setting a Thread Name (Unmanaged)": http://msdn2.microsoft.com/en-us/library/xcb2z8hs(vs.71).aspx
void debug_SetThreadName(const char* name)
{
	// we pass information to the debugger via a special exception it
	// swallows. if not running under one, bail now to avoid
	// "first chance exception" warnings.
	if(!IsDebuggerPresent())
		return;

	// presented by Jay Bazuzi (from the VC debugger team) at TechEd 1999.
	const struct ThreadNameInfo
	{
		DWORD type;
		const char* name;
		DWORD thread_id;	// any valid ID or -1 for current thread
		DWORD flags;
	}
	info = { 0x1000, name, (DWORD)-1, 0 };
	__try
	{
		RaiseException(0x406D1388, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR*)&info);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// if we get here, the debugger didn't handle the exception.
		// this happens if profiling with Dependency Walker; ENSURE
		// must not be called because we may be in critical init.
	}
}
