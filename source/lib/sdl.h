#ifndef _lib_sdl_H
#define _lib_sdl_H

#define SDL_BUTTON_INDEX_COUNT 5

#if OS_WIN && !defined(NO_WSDL)
# include "sysdep/win/wsdl.h"

// The SDL_BUTTON_* enum is zero-based and in the range [0..4] in wsdl.h
#define SDL_BUTTON_TO_INDEX(_but) _but
#define SDL_INDEX_TO_BUTTON(_idx) _idx

#else
# include <SDL/SDL.h>
# include <SDL/SDL_thread.h>
# include <SDL/SDL_endian.h>
	// if the compiler doesn't support inlining, this header will pull
	// in static bswap routines. doesn't matter - modern compilers
	// will strip them if unused, and this is more convenient than
	// another header that toggles between wsdl and SDL_endian.h.

#if SDL_BUTTON_LEFT == 1 && SDL_BUTTON_MIDDLE == 2 && SDL_BUTTON_RIGHT == 3 \
	&& SDL_BUTTON_WHEELUP == 4 && SDL_BUTTON_WHEELDOWN == 5

#define SDL_BUTTON_TO_INDEX(_but) ((_but) - 1) 
#define SDL_INDEX_TO_BUTTON(_idx) ((_idx) - 1)

#else

/* Add a test like the one above for your set of button constants, and implement
the BUTTON_TO_INDEX macros so that the resulting indices are within the range
[0, SDL_BUTTON_INDEX_COUNT-1]
*/
#error "Platform with unrecognized SDL constants. Update sdl.h"

#endif

#endif

#endif // _lib_sdl_H
