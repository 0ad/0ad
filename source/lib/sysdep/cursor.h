/**
 * =========================================================================
 * File        : cursor.h
 * Project     : 0 A.D.
 * Description : mouse cursor
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_SYSDEP_CURSOR
#define INCLUDED_SYSDEP_CURSOR

typedef void* sys_cursor;

/**
 * create a cursor from the given color image.
 *
 * @ w, h image dimensions [pixels]. the maximum value is
 * implementation-defined; 32x32 is typical and safe.
 * @param bgra_img cursor image (BGRA format, bottom-up).
 * it is copied and can be freed after this call returns.
 * @param hx,hy 'hotspot', i.e. offset from the upper-left corner to the
 * position where mouse clicks are registered.
 * @param cursor is 0 if the return code indicates failure, otherwise
 * a valid cursor that must be sys_cursor_free-ed when no longer needed.
 **/
extern LibError sys_cursor_create(int w, int h, void* bgra_img, int hx, int hy, sys_cursor* cursor);

/**
 * create a transparent cursor (used to hide the system cursor)
 *
 * @param cursor is 0 if the return code indicates failure, otherwise
 * a valid cursor that must be sys_cursor_free-ed when no longer needed.
 **/
extern LibError sys_cursor_create_empty(sys_cursor* cursor);

/**
 * override the current system cursor.
 *
 * @param cursor can be 0 to restore the default.
 **/
extern LibError sys_cursor_set(sys_cursor cursor);

/**
 * destroy the indicated cursor and frees its resources.
 *
 * @param cursor if currently in use, the default cursor is restored first.
 **/
extern LibError sys_cursor_free(sys_cursor cursor);

#endif	 // #ifndef INCLUDED_SYSDEP_CURSOR
