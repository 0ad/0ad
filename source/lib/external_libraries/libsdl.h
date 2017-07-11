/* Copyright (C) 2015 Wildfire Games.
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

# include "SDL.h"
# include "SDL_thread.h"

#if !SDL_VERSION_ATLEAST(2,0,2)
#error You are using an old libsdl release. At least libsdl2 >= 2.0.2 is required.
#endif

// if the compiler doesn't support inlining, this header will pull
// in static bswap routines. doesn't matter - modern compilers
// will strip them if unused, and this is more convenient than
// another header that toggles between wsdl and SDL_endian.h.
# include "SDL_endian.h"

# if MSC_VERSION
#  pragma comment(lib, "SDL2")
#  pragma comment(lib, "SDL2main")
# endif

// complete definition of our forward-declared SDL_Event (see sdl_fwd.h)
struct SDL_Event_
{
	SDL_Event ev;
};

#endif // INCLUDED_SDL
