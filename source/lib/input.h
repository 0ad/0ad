/**
 * =========================================================================
 * File        : input.h
 * Project     : 0 A.D.
 * Description : SDL input redirector; dispatches to multiple handlers and
 *             : allows record/playback of events.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2002 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef INPUT_H__
#define INPUT_H__


#include "lib/sdl_fwd.h"

// input handler return values.
enum InReaction
{
	// (the handlers' return values are checked and these
	// 'strange' values might bring errors to light)

	// pass the event to the next handler in the chain
	IN_PASS = 4,

	// we've handled it; no other handlers will receive this event.
	IN_HANDLED = 2
};

typedef InReaction (*InHandler)(const SDL_Event_*);

// register an input handler, which will receive all subsequent events first.
// events are passed to other handlers if handler returns IN_PASS.
extern void in_add_handler(InHandler handler);

// send event to each handler (newest first) until one returns true
extern void in_dispatch_event(const SDL_Event_* event);

extern void in_dispatch_recorded_events();

extern LibError in_record(const char* fn);
extern LibError in_playback(const char* fn);
extern void in_stop(void);

#endif	// #ifndef INPUT_H__
