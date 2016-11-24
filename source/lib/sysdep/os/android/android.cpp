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

#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/cursor.h"

#include "lib/external_libraries/libsdl.h"

Status sys_clipboard_set(const wchar_t* UNUSED(text))
{
	return INFO::OK;
}

wchar_t* sys_clipboard_get()
{
	return NULL;
}

Status sys_clipboard_free(wchar_t* UNUSED(copy))
{
	return INFO::OK;
}

namespace gfx {

Status GetVideoMode(int* xres, int* yres, int* bpp, int* freq)
{
#warning TODO: implement gfx::GetVideoMode properly for Android

	if(xres)
		*xres = 800;

	if(yres)
		*yres = 480;

	if(bpp)
		*bpp = 32;

	if(freq)
		*freq = 0;

	return INFO::OK;
}

}

// stub implementation of sys_cursor* functions

// note: do not return ERR_NOT_IMPLEMENTED or similar because that
// would result in WARN_ERRs.
Status sys_cursor_create(int w, int h, void* bgra_img, int hx, int hy, sys_cursor* cursor)
{
	UNUSED2(w);
	UNUSED2(h);
	UNUSED2(hx);
	UNUSED2(hy);
	UNUSED2(bgra_img);

	*cursor = 0;
	return INFO::OK;
}

// returns a dummy value representing an empty cursor
Status sys_cursor_create_empty(sys_cursor* cursor)
{
	*cursor = (void*)1; // any non-zero value, since the cursor NULL has special meaning
	return INFO::OK;
}

// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
Status sys_cursor_set(sys_cursor cursor)
{
	if (cursor) // dummy empty cursor
		SDL_ShowCursor(SDL_DISABLE);
	else // restore default cursor
		SDL_ShowCursor(SDL_ENABLE);

	return INFO::OK;
}

// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
Status sys_cursor_free(sys_cursor cursor)
{
	// bail now to prevent potential confusion below; there's nothing to do.
	if(!cursor)
		return INFO::OK;

	SDL_ShowCursor(SDL_ENABLE);

	return INFO::OK;
}

Status sys_cursor_reset()
{
	return INFO::OK;
}

