/* Copyright (C) 2010 Wildfire Games.
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
 * forward declaration of SDL_Event
 */

#ifndef INCLUDED_SDL_FWD
#define INCLUDED_SDL_FWD

// 2006-08-26 SDL is dragged into 6 of our 7 static library components.
// it must be specified in each of their "extern_libs" so that the
// include path is set and <SDL.h> can be found.
//
// obviously this is bad, so we work around the root cause. mostly only
// SDL_Event is needed. unfortunately it cannot be forward-declared,
// because it is a union (regrettable design mistake).
// we fix this by wrapping it in a struct, which can safely be
// forward-declared and used for SDL_Event_* parameters.
struct SDL_Event_;

#endif	// #ifndef INCLUDED_SDL_FWD
