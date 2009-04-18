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

#include "precompiled.h"
#include "lib/sysdep/cursor.h"

#include "win.h"
#include "wutil.h"


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
