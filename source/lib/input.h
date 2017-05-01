/* Copyright (C) 2017 Wildfire Games.
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
 * SDL input redirector; dispatches to multiple handlers.
 */

#ifndef INCLUDED_INPUT
#define INCLUDED_INPUT


#include "lib/external_libraries/libsdl_fwd.h"

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

// remove all registered input handlers
extern void in_reset_handlers();

// send event to each handler (newest first) until one returns true
extern void in_dispatch_event(const SDL_Event_* event);

// push an event onto the back of a high-priority queue - the new event will
// be returned by in_poll_event before any standard SDL events
extern void in_push_priority_event(const SDL_Event_* event);

// reads events that were pushed by in_push_priority_event
// returns 1 if an event was read, 0 otherwise.
extern int in_poll_priority_event(SDL_Event_* event);

// reads events that were pushed by in_push_priority_event, or, if there are
// no high-priority events) reads from the SDL event queue with SDL_PollEvent.
// returns 1 if an event was read, 0 otherwise.
extern int in_poll_event(SDL_Event_* event);

#endif	// #ifndef INCLUDED_INPUT
