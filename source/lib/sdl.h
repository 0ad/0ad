/**
 * =========================================================================
 * File        : sdl.h
 * Project     : 0 A.D.
 * Description : SDL header; uses emulator on Windows, otherwise libsdl.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef LIB_SDL_H__
#define LIB_SDL_H__

#if OS_WIN && !defined(CONFIG_NO_WSDL)
# include "lib/sysdep/win/wsdl.h"
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
