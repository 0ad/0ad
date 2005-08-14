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

#include "precompiled.h"

#include "input.h"
#include "sdl.h"
#include "lib.h"

#include <stdio.h>
#include <stdlib.h>


#define MAX_HANDLERS 8

static EventHandler handler_stack[MAX_HANDLERS];
static int handler_stack_top = 0;


int in_add_handler(EventHandler handler)
{
	if(handler_stack_top >= MAX_HANDLERS || !handler)
	{
		debug_warn("in_add_handler");
		return -1;
	}

	handler_stack[handler_stack_top++] = handler;

	return 0;
}


// send event to each handler (newest first) until one returns true
void dispatch_event(const SDL_Event* event)
{
	for(int i = handler_stack_top-1; i >= 0; i--)
	{
		int ret = handler_stack[i](event);
		// .. done, return
		if(ret == EV_HANDLED)
			return;
		// .. next handler
		else if(ret == EV_PASS)
			continue;
		// .. invalid return value
		else
			debug_warn("dispatch_event: invalid handler return value");
	}
}



static enum
{
	INIT,		// first call to in_record() or in_playback(): register cleanup routine
	IDLE,
	RECORD,
	PLAYBACK
}
state = INIT;

static FILE* f;

u32 game_ticks;

static u32 time_adjust = 0;
static u32 next_event_time;


void in_stop()
{
	if(f)
	{
		fclose(f);
		f = 0;
	}

	state = IDLE;
}


int in_record(const char* fn)
{
	if(state == INIT)
		atexit(in_stop);

	in_stop();

	f = fopen(fn, "wb");
	if(!f)
		return -1;

	fwrite(&game_ticks, sizeof(u32), 1, f);

	state = RECORD;

	return 0;
}


int in_playback(const char* fn)
{
	if(state == INIT)
		atexit(in_stop);

	in_stop();

	f = fopen(fn, "rb");
	if(!f)
		return -1;

	u32 rec_start_time;
	fread(&rec_start_time, sizeof(u32), 1, f);
	time_adjust = game_ticks-rec_start_time;

	fread(&next_event_time, sizeof(u32), 1, f);
	next_event_time += time_adjust;

	state = PLAYBACK;

	return 0;
}



void in_dispatch_event(const SDL_Event* event)
{
	if(state == RECORD)
	{
		fwrite(&game_ticks, sizeof(u32), 1, f);
		fwrite(event, sizeof(SDL_Event), 1, f);
	}

	dispatch_event(event);
}


void in_dispatch_recorded_events()
{
	SDL_Event event;

	while(state == PLAYBACK && next_event_time <= game_ticks)
	{
		fread(&event, sizeof(SDL_Event), 1, f);

		// do this before dispatch_event(),
		// in case a handler calls in_stop() (setting f to 0)
		if(!fread(&next_event_time, sizeof(u32), 1, f))
{
			in_stop();
exit(0x73c07d);
// TODO: 'disconnect'?
}
		next_event_time += time_adjust;			

		in_dispatch_event(&event);	
	}
}
