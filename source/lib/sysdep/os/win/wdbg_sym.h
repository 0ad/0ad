/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
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
 * iterate over a call stack, invoking a callback for each frame encountered.
 *
 * @param pcontext processor context from which to start (usually taken from
 * an exception record), or 0 to walk the current stack.
 *
 * note: it is safe to use debug_assert/debug_warn/CHECK_ERR even during a
 * stack trace (which is triggered by debug_assert et al. in app code) because
 * nested stack traces are ignored and only the error is displayed.
 **/
extern LibError wdbg_sym_WalkStack(StackFrameCallback cb, uintptr_t cbData = 0, size_t skip = 0, const _CONTEXT* pcontext = 0);

extern void wdbg_sym_WriteMinidump(_EXCEPTION_POINTERS* ep);

#endif	// #ifndef INCLUDED_WDBG_SYM
