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

#include "precompiled.h"
#include "input.h"

#include <stdio.h>
#include <stdlib.h>

#include "lib/external_libraries/libsdl.h"

const size_t MAX_HANDLERS = 9;
static InHandler handler_stack[MAX_HANDLERS];
static size_t handler_stack_top = 0;

static std::list<SDL_Event_> priority_events;

void in_add_handler(InHandler handler)
{
	ENSURE(handler);

	if(handler_stack_top >= MAX_HANDLERS)
		WARN_IF_ERR(ERR::LIMIT);

	handler_stack[handler_stack_top++] = handler;
}

void in_reset_handlers()
{
	handler_stack_top = 0;
}

// send ev to each handler until one returns IN_HANDLED
void in_dispatch_event(const SDL_Event_* ev)
{
	for(int i = (int)handler_stack_top-1; i >= 0; i--)
	{
		ENSURE(handler_stack[i] && ev);
		InReaction ret = handler_stack[i](ev);
		// .. done, return
		if(ret == IN_HANDLED)
			return;
		// .. next handler
		else if(ret == IN_PASS)
			continue;
		// .. invalid return value
		else
			DEBUG_WARN_ERR(ERR::LOGIC);	// invalid handler return value
	}
}

void in_push_priority_event(const SDL_Event_* event)
{
	priority_events.push_back(*event);
}

int in_poll_priority_event(SDL_Event_* event)
{
	if (priority_events.empty())
		return 0;

	*event = priority_events.front();
	priority_events.pop_front();
	return 1;
}

int in_poll_event(SDL_Event_* event)
{
	return in_poll_priority_event(event) ? 1 : SDL_PollEvent(&event->ev);
}
