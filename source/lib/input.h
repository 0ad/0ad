/*
 * input layer (dispatch events to multiple handlers; record/playback events)
 *
 * Copyright (c) 2002 Jan Wassenberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Contact info:
 *   Jan.Wassenberg@stud.uni-karlsruhe.de
 *   http://www.stud.uni-karlsruhe.de/~urkt/
 */

#ifndef INPUT_H__
#define INPUT_H__

#include "sdl.h"

#ifdef __cplusplus
extern "C" {
#endif


// event handler return value defs (int).
// don't require an enum type - simplifies user function decl;
// the dispatcher makes sure each return value is correct.
enum
{
	// pass the event to the next handler in the chain
	EV_PASS = 4,

	// we've handled it; no other handlers will receive this event.
	EV_HANDLED = 2
};

// declare functions to take SDL_Event*; in_add_handler converts to void*
// (avoids header dependency on SDL)
typedef int (*EventHandler)(const SDL_Event*);


// register an input handler, which will receive all subsequent events first.
// events are passed to other handlers if handler returns EV_PASS.
extern int in_add_handler(EventHandler handler);

// send event to each handler (newest first) until one returns true
extern void in_dispatch_event(const SDL_Event* event);

extern void in_dispatch_recorded_events();

extern int in_record(const char* fn);
extern int in_playback(const char* fn);
extern void in_stop(void);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef INPUT_H__
