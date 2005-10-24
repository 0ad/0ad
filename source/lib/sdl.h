#ifndef LIB_SDL_H__
#define LIB_SDL_H__

#if OS_WIN && !defined(CONFIG_NO_WSDL)
# include "sysdep/win/wsdl.h"
#else

# include <SDL/SDL.h>
# include <SDL/SDL_thread.h>
# include <SDL/SDL_endian.h>
	// if the compiler doesn't support inlining, this header will pull
	// in static bswap routines. doesn't matter - modern compilers
	// will strip them if unused, and this is more convenient than
	// another header that toggles between wsdl and SDL_endian.h.

#endif

#endif // LIB_SDL_H__
