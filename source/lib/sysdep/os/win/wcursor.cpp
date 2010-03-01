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

#include "precompiled.h"
#include "lib/sysdep/cursor.h"

#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/wutil.h"


static sys_cursor cursor_from_HICON(HICON hIcon)
{
	return (sys_cursor)(uintptr_t)hIcon;
}

static sys_cursor cursor_from_HCURSOR(HCURSOR hCursor)
{
	return (sys_cursor)(uintptr_t)hCursor;
}

static HICON HICON_from_cursor(sys_cursor cursor)
{
	return (HICON)(uintptr_t)cursor;
}

static HCURSOR HCURSOR_from_cursor(sys_cursor cursor)
{
	return (HCURSOR)(uintptr_t)cursor;
}


LibError sys_cursor_create(int w, int h, void* bgra_img, int hx, int hy, sys_cursor* cursor)
{
	*cursor = 0;

	// MSDN says selecting this HBITMAP into a DC is slower since we use
	// CreateBitmap; bpp/format must be checked against those of the DC.
	// this is the simplest way and we don't care about slight performance
	// differences because this is typically only called once.
	HBITMAP hbmColour = CreateBitmap(w, h, 1, 32, bgra_img);

	// CreateIconIndirect doesn't access this; we just need to pass
	// an empty bitmap.
	HBITMAP hbmMask = CreateBitmap(w, h, 1, 1, 0);

	// create the cursor (really an icon; they differ only in
	// fIcon and the hotspot definitions).
	ICONINFO ii;
	ii.fIcon = FALSE;  // cursor
	ii.xHotspot = (DWORD)hx;
	ii.yHotspot = (DWORD)hy;
	ii.hbmMask  = hbmMask;
	ii.hbmColor = hbmColour;
	HICON hIcon = CreateIconIndirect(&ii);

	// CreateIconIndirect makes copies, so we no longer need these.
	DeleteObject(hbmMask);
	DeleteObject(hbmColour);

	if(!wutil_IsValidHandle(hIcon))
		WARN_RETURN(ERR::FAIL);

	*cursor = cursor_from_HICON(hIcon);
	return INFO::OK;
}


LibError sys_cursor_create_empty(sys_cursor* cursor)
{
	u8 bgra_img[] = {0, 0, 0, 0};
	return sys_cursor_create(1, 1, bgra_img, 0, 0, cursor);
}


LibError sys_cursor_set(sys_cursor cursor)
{
	// restore default cursor.
	if(!cursor)
		cursor = cursor_from_HCURSOR(LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW)));

	(void)SetCursor(HCURSOR_from_cursor(cursor));
	// return value (previous cursor) is useless.

	return INFO::OK;
}


LibError sys_cursor_free(sys_cursor cursor)
{
	// bail now to prevent potential confusion below; there's nothing to do.
	if(!cursor)
		return INFO::OK;

	// if the cursor being freed is active, restore the default arrow
	// (just for safety).
	if(cursor_from_HCURSOR(GetCursor()) == cursor)
		WARN_ERR(sys_cursor_set(0));

	BOOL ok = DestroyIcon(HICON_from_cursor(cursor));
	return LibError_from_win32(ok);
}
