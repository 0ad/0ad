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
		memmove(gfx_card+chars_to_keep, gfx_card+ARRAY_SIZE(what)-1, (wcslen(gfx_card)-ARRAY_SIZE(what))*sizeof(wchar_t));
	SHORTEN(L"ATI Technologies Inc.", 3);
	SHORTEN(L"NVIDIA Corporation", 6);
	SHORTEN(L"S3 Graphics", 2);					// returned by EnumDisplayDevices
	SHORTEN(L"S3 Graphics, Incorporated", 2);	// returned by GL_VENDOR
#undef SHORTEN
}
