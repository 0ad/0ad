#if defined(_WIN32) && !defined(NO_WSDL)
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
