/**
 * =========================================================================
 * File        : input.cpp
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

#include "precompiled.h"

#include "lib.h"
#include "input.h"

#include <stdio.h>
#include <stdlib.h>


const uint MAX_HANDLERS = 8;
static InHandler handler_stack[MAX_HANDLERS];
static uint handler_stack_top = 0;

void in_add_handler(InHandler handler)
{
	debug_assert(handler);

	if(handler_stack_top >= MAX_HANDLERS)
		WARN_ERR_RETURN(ERR_LIMIT);

	handler_stack[handler_stack_top++] = handler;
}


// send event to each handler until one returns IN_HANDLED
static void dispatch_event(const SDL_Event* event)
{
	for(int i = (int)handler_stack_top-1; i >= 0; i--)
	{
		debug_assert(handler_stack[i] && event);
		InReaction ret = handler_stack[i](event);
		// .. done, return
		if(ret == IN_HANDLED)
			return;
		// .. next handler
		else if(ret == IN_PASS)
			continue;
		// .. invalid return value
		else
			debug_warn("invalid handler return value");
	}
}


//-----------------------------------------------------------------------------

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


LibError in_record(const char* fn)
{
	if(state == INIT)
		atexit(in_stop);

	in_stop();

	f = fopen(fn, "wb");
	if(!f)
		WARN_RETURN(ERR_FILE_ACCESS);

	fwrite(&game_ticks, sizeof(u32), 1, f);

	state = RECORD;

	return INFO_OK;
}


LibError in_playback(const char* fn)
{
	if(state == INIT)
		atexit(in_stop);

	in_stop();

	f = fopen(fn, "rb");
	if(!f)
		WARN_RETURN(ERR_TNODE_NOT_FOUND);

	u32 rec_start_time;
	fread(&rec_start_time, sizeof(u32), 1, f);
	time_adjust = game_ticks-rec_start_time;

	fread(&next_event_time, sizeof(u32), 1, f);
	next_event_time += time_adjust;

	state = PLAYBACK;

	return INFO_OK;
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
