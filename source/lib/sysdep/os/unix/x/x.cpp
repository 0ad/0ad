/* Copyright (C) 2020 Wildfire Games.
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

// X Window System-specific code

#include "precompiled.h"

#if OS_LINUX || OS_BSD
# define HAVE_X 1
#else
# define HAVE_X 0
#endif

#if HAVE_X

#include "lib/debug.h"
#include "lib/utf8.h"
#include "lib/sysdep/gfx.h"

#include "ps/VideoMode.h"

#include <X11/Xlib.h>
#include <stdlib.h>
#include <X11/Xatom.h>

#include "SDL.h"
#include "SDL_syswm.h"

#include <algorithm>
#undef Status

static Display *g_SDL_Display;
static Window g_SDL_Window;
static wchar_t *selection_data=NULL;
static size_t selection_size=0;

namespace gfx {

Status GetVideoMode(int* xres, int* yres, int* bpp, int* freq)
{
	Display* disp = XOpenDisplay(0);
	if(!disp)
		WARN_RETURN(ERR::FAIL);

	int screen = XDefaultScreen(disp);

	/* 2004-07-13
	NOTE: The XDisplayWidth/Height functions don't actually return the current
	display mode - they return the size of the root window. This means that
	users with "Virtual Desktops" bigger than what their monitors/graphics
	card can handle will have to set their 0AD screen resolution manually.

	There's supposed to be an X extension that can give you the actual display
	mode, probably including refresh rate info etc, but it's not worth
	researching and implementing that at this stage.
	*/

	if(xres)
		*xres = XDisplayWidth(disp, screen);
	if(yres)
		*yres = XDisplayHeight(disp, screen);
	if(bpp)
		*bpp = XDefaultDepth(disp, screen);
	if(freq)
		*freq = 0;
	XCloseDisplay(disp);
	return INFO::OK;
}

}	// namespace gfx


static bool get_wminfo(SDL_SysWMinfo& wminfo)
{
	SDL_VERSION(&wminfo.version);

	const int ret = SDL_GetWindowWMInfo(g_VideoMode.GetWindow(), &wminfo);

	if(ret == 1)
		return true;

	if(ret == -1)
	{
		debug_printf("SDL_GetWMInfo failed\n");
		return false;
	}
	if(ret == 0)
	{
		debug_printf("SDL_GetWMInfo is not implemented on this platform\n");
		return false;
	}

	debug_printf("SDL_GetWMInfo returned an unknown value: %d\n", ret);
	return false;
}

#endif	// #if HAVE_X
