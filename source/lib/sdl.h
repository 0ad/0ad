#if defined(_WIN32) && !defined(NO_WSDL)
# include "sysdep/win/wsdl.h"
#else
# include <SDL/SDL.h>
# include <SDL/SDL_thread.h>
#endif
