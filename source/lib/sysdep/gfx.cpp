/**
 * =========================================================================
 * File        : gfx.cpp
 * Project     : 0 A.D.
 * Description : graphics card detection.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "gfx.h"

#include "lib/external_libraries/sdl.h"
#include "lib/ogl.h"


char gfx_card[GFX_CARD_LEN] = "";
char gfx_drv_ver[GFX_DRV_VER_LEN] = "";

int gfx_mem = -1;	// [MiB]; approximate


// detect graphics card and set the above information.
void gfx_detect()
{
	// TODO: add sizeof(FB)?
	gfx_mem = (SDL_GetVideoInfo()->video_mem) / 1048576;	// [MiB]

	// try platform-specific version: they return more
	// detailed information, and don't require OpenGL to be ready.
#if OS_WIN
	extern LibError win_get_gfx_info();
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
	if(!strncmp(gfx_card, what, ARRAY_SIZE(what)-1))\
	memmove(gfx_card+chars_to_keep, gfx_card+ARRAY_SIZE(what)-1, strlen(gfx_card)-(ARRAY_SIZE(what)-1)+1);
	SHORTEN("ATI Technologies Inc.", 3);
	SHORTEN("NVIDIA Corporation", 6);
	SHORTEN("S3 Graphics", 2);					// returned by EnumDisplayDevices
	SHORTEN("S3 Graphics, Incorporated", 2);	// returned by GL_VENDOR
#undef SHORTEN
}
