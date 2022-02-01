/* Copyright (C) 2022 Wildfire Games.
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

#ifdef _MSC_VER
# pragma warning(disable:4091) // hides previous local declaration
#endif

#include "lib/sysdep/os/win/win.h"	// CONTEXT, EXCEPTION_POINTERS

struct _tagSTACKFRAME64;

/**
 * called for each stack frame found by wdbg_sym_WalkStack.
 *
 * @param frame the dbghelp stack frame (we can't just pass the
 *   instruction-pointer because dump_frame_cb needs the frame pointer to
 *   locate frame-relative variables)
 * @param cbData the user-specified value that was passed to wdbg_sym_WalkStack
 * @return Status (see RETURN_STATUS_FROM_CALLBACK).
 **/
typedef Status (*StackFrameCallback)(const _tagSTACKFRAME64* frame, uintptr_t cbData);

/**
 * Iterate over a call stack, invoking a callback for each frame encountered.
 *
 * @param cb
 * @param cbData
 * @param context Processor context from which to start (taken from
 *   an exception record or debug_CaptureContext).
 * @param lastFuncToSkip
 *
 * @note It is safe to use ENSURE/debug_warn/WARN_RETURN_STATUS_IF_ERR even during a
 *   stack trace (which is triggered by ENSURE et al. in app code) because
 *   nested stack traces are ignored and only the error is displayed.
 **/
Status wdbg_sym_WalkStack(StackFrameCallback cb, uintptr_t cbData, CONTEXT& context, const wchar_t* lastFuncToSkip = 0);

void wdbg_sym_WriteMinidump(EXCEPTION_POINTERS* ep);

#endif	// #ifndef INCLUDED_WDBG_SYM
