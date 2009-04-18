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
 * SDL header; uses emulator on Windows, otherwise libsdl.
 */

#ifndef INCLUDED_SDL
#define INCLUDED_SDL

#include "sdl_fwd.h"
#include "lib/config2.h"	// CONFIG2_WSDL

#if OS_WIN && CONFIG2_WSDL
# include "lib/sysdep/os/win/wsdl.h"
#else

# include <SDL/SDL.h>
# include <SDL/SDL_thread.h>

// if the compiler doesn't support inlining, this header will pull
// in static bswap routines. doesn't matter - modern compilers
// will strip them if unused, and this is more convenient than
// another header that toggles between wsdl and SDL_endian.h.
# include <SDL/SDL_endian.h>

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
