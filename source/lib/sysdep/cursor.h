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
 * mouse cursor
 */

#ifndef INCLUDED_SYSDEP_CURSOR
#define INCLUDED_SYSDEP_CURSOR

typedef void* sys_cursor;

/**
 * Create a cursor from the given color image.
 *
 * @param w,h Image dimensions [pixels]. the maximum value is
 *		  implementation-defined; 32x32 is typical and safe.
 * @param bgra_img cursor image (BGRA format, bottom-up).
 *		  It is copied and can be freed after this call returns.
 * @param hx,hy 'hotspot', i.e. offset from the upper-left corner to the
 *		  position where mouse clicks are registered.
 * @param cursor Is 0 if the return code indicates failure, otherwise
 *		  a valid cursor that must be sys_cursor_free-ed when no longer needed.
 **/
extern Status sys_cursor_create(int w, int h, void* bgra_img, int hx, int hy, sys_cursor* cursor);

/**
 * Create a transparent cursor (used to hide the system cursor).
 *
 * @param cursor is 0 if the return code indicates failure, otherwise
 * a valid cursor that must be sys_cursor_free-ed when no longer needed.
 **/
extern Status sys_cursor_create_empty(sys_cursor* cursor);

/**
 * override the current system cursor.
 *
 * @param cursor can be 0 to restore the default.
 **/
extern Status sys_cursor_set(sys_cursor cursor);

/**
 * destroy the indicated cursor and frees its resources.
 *
 * @param cursor if currently in use, the default cursor is restored first.
 **/
extern Status sys_cursor_free(sys_cursor cursor);

/**
 * reset any cached cursor data.
 * on some systems, this is needed when resetting the SDL video subsystem.
 **/
extern Status sys_cursor_reset();

#endif	 // #ifndef INCLUDED_SYSDEP_CURSOR
