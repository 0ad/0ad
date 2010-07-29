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

#ifndef INCLUDED_WDBG_SYM
#define INCLUDED_WDBG_SYM

struct _tagSTACKFRAME64;
struct _CONTEXT;
struct _EXCEPTION_POINTERS;

/**
 * called for each stack frame found by wdbg_sym_WalkStack.
 *
 * @param frame the dbghelp stack frame (we can't just pass the
 * instruction-pointer because dump_frame_cb needs the frame pointer to
 * locate frame-relative variables)
 * @param cbData the user-specified value that was passed to wdbg_sym_WalkStack
 * @return INFO::CB_CONTINUE to continue, anything else to stop immediately
 * and return that value to wdbg_sym_WalkStack's caller.
 **/
typedef LibError (*StackFrameCallback)(const _tagSTACKFRAME64* frame, uintptr_t cbData);

/**
 * Iterate over a call stack, invoking a callback for each frame encountered.
 *
 * @param cb
 * @param cbData
 * @param pcontext Processor context from which to start (usually taken from
 *		  an exception record), or 0 to walk the current stack.
 * @param lastFuncToSkip
 *
 * @note It is safe to use debug_assert/debug_warn/CHECK_ERR even during a
 * stack trace (which is triggered by debug_assert et al. in app code) because
 * nested stack traces are ignored and only the error is displayed.
 **/
extern LibError wdbg_sym_WalkStack(StackFrameCallback cb, uintptr_t cbData = 0, const _CONTEXT* pcontext = 0, const wchar_t* lastFuncToSkip = 0);

extern void wdbg_sym_WriteMinidump(_EXCEPTION_POINTERS* ep);

#endif	// #ifndef INCLUDED_WDBG_SYM
