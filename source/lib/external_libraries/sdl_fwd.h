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
 * forward declaration of SDL_Event
 */

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
