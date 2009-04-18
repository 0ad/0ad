/**
 * =========================================================================
 * File        : sdl_fwd.h
 * Project     : 0 A.D.
 * Description : forward declaration of SDL_Event
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_SDL_FWD
#define INCLUDED_SDL_FWD

// 2006-08-26 SDL is dragged into 6 of our 7 static library components.
// it must be specified in each of their "extern_libs" so that the
// include path is set and <SDL/sdl.h> can be found.
//
// obviously this is bad, so we work around the root cause. mostly only
// SDL_Event is needed. unfortunately it cannot be forward-declared,
// because it is a union (regrettable design mistake).
// we fix this by wrapping it in a struct, which can safely be
// forward-declared and used for SDL_Event_* parameters.
struct SDL_Event_;

#endif	// #ifndef INCLUDED_SDL_FWD
