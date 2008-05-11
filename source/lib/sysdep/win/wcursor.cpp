#include "precompiled.h"
#include "../cursor.h"

#include "win.h"
#include "wutil.h"


static void* ptr_from_HICON(HICON hIcon)
{
	return (void*)(uintptr_t)hIcon;
}

static void* ptr_from_HCURSOR(HCURSOR hCursor)
{
	return (void*)(uintptr_t)hCursor;
}

static HICON HICON_from_ptr(void* p)
{
	return (HICON)(uintptr_t)p;
}

static HCURSOR HCURSOR_from_ptr(void* p)
{
	return (HCURSOR)(uintptr_t)p;
}


// creates a cursor from the given image.
// w, h specify image dimensions [pixels]. limit is implementation-
//   dependent; 32x32 is typical and safe.
// bgra_img is the cursor image (BGRA format, bottom-up).
//   it is no longer needed and can be freed after this call returns.
// hotspot (hx,hy) is the offset from its upper-left corner to the
//   position where mouse clicks are registered.
// cursor is only valid when INFO::OK is returned; in that case, it must be
//   sys_cursor_free-ed when no longer needed.
LibError sys_cursor_create(int w, int h, void* bgra_img, int hx, int hy, void** cursor)
{
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

	if(!hIcon)	// not INVALID_HANDLE_VALUE
		WARN_RETURN(ERR::FAIL);

	*cursor = ptr_from_HICON(hIcon);
	return INFO::OK;
}

LibError sys_cursor_create_empty(void **cursor)
{
	u8 bgra_img[] = {0, 0, 0, 0};
	return sys_cursor_create(1, 1, bgra_img, 0, 0, cursor);
}

// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
LibError sys_cursor_set(void* cursor)
{
	// restore default cursor.
	if(!cursor)
		cursor = ptr_from_HCURSOR(LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW)));

	(void)SetCursor(HCURSOR_from_ptr(cursor));
	// return value (previous cursor) is useless.

	return INFO::OK;
}


// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
LibError sys_cursor_free(void* cursor)
{
	// bail now to prevent potential confusion below; there's nothing to do.
	if(!cursor)
		return INFO::OK;

	// if the cursor being freed is active, restore the default arrow
	// (just for safety).
	if(ptr_from_HCURSOR(GetCursor()) == cursor)
		WARN_ERR(sys_cursor_set(0));

	BOOL ok = DestroyIcon(HICON_from_ptr(cursor));
	return LibError_from_win32(ok);
}
