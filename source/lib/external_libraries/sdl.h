/**
 * =========================================================================
 * File        : sdl.h
 * Project     : 0 A.D.
 * Description : SDL header; uses emulator on Windows, otherwise libsdl.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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
