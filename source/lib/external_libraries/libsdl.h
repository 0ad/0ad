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

/*
 * SDL header; uses emulator on Windows, otherwise libsdl.
 */

#ifndef INCLUDED_SDL
#define INCLUDED_SDL

#include "lib/external_libraries/libsdl_fwd.h"
#include "lib/config2.h"	// CONFIG2_WSDL

#if OS_WIN && CONFIG2_WSDL
# include "lib/sysdep/os/win/wsdl.h"
#else

# include "SDL.h"
# include "SDL_thread.h"

// if the compiler doesn't support inlining, this header will pull
// in static bswap routines. doesn't matter - modern compilers
// will strip them if unused, and this is more convenient than
// another header that toggles between wsdl and SDL_endian.h.
# include "SDL_endian.h"

# if MSC_VERSION
#  pragma comment(lib, "SDL")
#  pragma comment(lib, "SDLmain")
# endif

#endif

// complete definition of our forward-declared SDL_Event (see sdl_fwd.h)
struct SDL_Event_
{
	SDL_Event ev;
};

#endif // INCLUDED_SDL
