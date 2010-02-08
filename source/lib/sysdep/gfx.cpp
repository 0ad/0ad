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
 * graphics card detection.
 */

#include "precompiled.h"
#include "gfx.h"

#include "lib/external_libraries/sdl.h"
#include "lib/ogl.h"
#if OS_WIN
# include "lib/sysdep/os/win/wgfx.h"
#endif


wchar_t gfx_card[GFX_CARD_LEN] = L"";
wchar_t gfx_drv_ver[GFX_DRV_VER_LEN] = L"";

int gfx_mem = -1;	// [MiB]; approximate


// detect graphics card and set the above information.
void gfx_detect()
{
	// TODO: add sizeof(FB)?
	gfx_mem = (SDL_GetVideoInfo()->video_mem) / 1048576;	// [MiB]

	// try platform-specific version: they return more
	// detailed information, and don't require OpenGL to be ready.
#if OS_WIN
	if(win_get_gfx_info() < 0)
#endif
	{
		// the OpenGL version should always work, unless OpenGL isn't ready for use,
		// or we were called between glBegin and glEnd.
		ogl_get_gfx_info();
	}

	// remove crap from vendor names. (don't dare touch the model name -
	// it's too risky, there are too many different strings)
#define SHORTEN(what, chars_to_keep)\
	if(!wcsncmp(gfx_card, what, ARRAY_SIZE(what)-1))\
		memmove(gfx_card+chars_to_keep, gfx_card+ARRAY_SIZE(what)-1, (wcslen(gfx_card)-(ARRAY_SIZE(what)-1)+1)*sizeof(wchar_t));
	SHORTEN(L"ATI Technologies Inc.", 3);
	SHORTEN(L"NVIDIA Corporation", 6);
	SHORTEN(L"S3 Graphics", 2);					// returned by EnumDisplayDevices
	SHORTEN(L"S3 Graphics, Incorporated", 2);	// returned by GL_VENDOR
#undef SHORTEN
}
