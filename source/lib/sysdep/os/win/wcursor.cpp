/* Copyright (c) 2012 Wildfire Games
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

#include "lib/sysdep/gfx.h"
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


static Status sys_cursor_create_common(int w, int h, void* bgra_img, void* mask_img, int hx, int hy, sys_cursor* cursor)
{
	*cursor = 0;

	// MSDN says selecting this HBITMAP into a DC is slower since we use
	// CreateBitmap; bpp/format must be checked against those of the DC.
	// this is the simplest way and we don't care about slight performance
	// differences because this is typically only called once.
	HBITMAP hbmColor = CreateBitmap(w, h, 1, 32, bgra_img);

	// CreateIconIndirect doesn't access this; we just need to pass
	// an empty bitmap.
	HBITMAP hbmMask = CreateBitmap(w, h, 1, 1, mask_img);

	// create the cursor (really an icon; they differ only in
	// fIcon and the hotspot definitions).
	ICONINFO ii;
	ii.fIcon = FALSE;  // cursor
	ii.xHotspot = (DWORD)hx;
	ii.yHotspot = (DWORD)hy;
	ii.hbmMask  = hbmMask;
	ii.hbmColor = hbmColor;
	HICON hIcon = CreateIconIndirect(&ii);

	// CreateIconIndirect makes copies, so we no longer need these.
	DeleteObject(hbmMask);
	DeleteObject(hbmColor);

	if(!wutil_IsValidHandle(hIcon))
		WARN_RETURN(ERR::FAIL);

	*cursor = cursor_from_HICON(hIcon);
	return INFO::OK;
}

Status sys_cursor_create(int w, int h, void* bgra_img, int hx, int hy, sys_cursor* cursor)
{
	// alpha-blended cursors do not work on a 16-bit display
	// (they get drawn as a black square), so refuse to load the
	// cursor in that case
	int bpp = 0;
	RETURN_STATUS_IF_ERR(gfx::GetVideoMode(NULL, NULL, &bpp, NULL));
	if (bpp <= 16)
		return ERR::FAIL;

	return sys_cursor_create_common(w, h, bgra_img, NULL, hx, hy, cursor);
}

Status sys_cursor_create_empty(sys_cursor* cursor)
{
	// the mask gets ignored on 32-bit displays, but is used on 16-bit displays;
	// setting it to 0xFF makes the cursor invisible (though I'm not quite
	// sure why it's that way round)
	u8 bgra_img[] = {0, 0, 0, 0};
	u8 mask_img[] = {0xFF};
	return sys_cursor_create_common(1, 1, bgra_img, mask_img, 0, 0, cursor);
}


Status sys_cursor_set(sys_cursor cursor)
{
	// restore default cursor.
	if(!cursor)
		cursor = cursor_from_HCURSOR(LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW)));

	(void)SetCursor(HCURSOR_from_cursor(cursor));
	// return value (previous cursor) is useless.

	return INFO::OK;
}


Status sys_cursor_free(sys_cursor cursor)
{
	// bail now to prevent potential confusion below; there's nothing to do.
	if(!cursor)
		return INFO::OK;

	// if the cursor being freed is active, restore the default arrow
	// (just for safety).
	if(cursor_from_HCURSOR(GetCursor()) == cursor)
		WARN_IF_ERR(sys_cursor_set(0));

	if(!DestroyIcon(HICON_from_cursor(cursor)))
		WARN_RETURN(StatusFromWin());
	return INFO::OK;
}

Status sys_cursor_reset()
{
	return INFO::OK;
}
