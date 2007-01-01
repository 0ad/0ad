/**
 * =========================================================================
 * File        : input.cpp
 * Project     : 0 A.D.
 * Description : SDL input redirector; dispatches to multiple handlers and
 *             : allows record/playback of evs.
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
#include "input.h"

#include <stdio.h>
#include <stdlib.h>

#include "lib.h"
#include "sdl.h"
#include "lib/res/file/file.h"

const uint MAX_HANDLERS = 8;
static InHandler handler_stack[MAX_HANDLERS];
static uint handler_stack_top = 0;

void in_add_handler(InHandler handler)
{
	debug_assert(handler);

	if(handler_stack_top >= MAX_HANDLERS)
		WARN_ERR_RETURN(ERR::LIMIT);

	handler_stack[handler_stack_top++] = handler;
}


// send ev to each handler until one returns IN_HANDLED
static void dispatch_ev(const SDL_Event_* ev)
{
	for(int i = (int)handler_stack_top-1; i >= 0; i--)
	{
		debug_assert(handler_stack[i] && ev);
		InReaction ret = handler_stack[i](ev);
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
static u32 next_ev_time;


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
		WARN_RETURN(ERR::FILE_ACCESS);

	fwrite(&game_ticks, sizeof(u32), 1, f);

	state = RECORD;

	return INFO::OK;
}


LibError in_playback(const char* fn)
{
	if(state == INIT)
		atexit(in_stop);

	in_stop();

	f = fopen(fn, "rb");
	if(!f)
		WARN_RETURN(ERR::FILE_ACCESS);

	u32 rec_start_time;
	fread(&rec_start_time, sizeof(u32), 1, f);
	time_adjust = game_ticks-rec_start_time;

	fread(&next_ev_time, sizeof(u32), 1, f);
	next_ev_time += time_adjust;

	state = PLAYBACK;

	return INFO::OK;
}



void in_dispatch_event(const SDL_Event_* ev)
{
	if(state == RECORD)
	{
		fwrite(&game_ticks, sizeof(u32), 1, f);
		fwrite(ev, sizeof(SDL_Event_), 1, f);
	}

	dispatch_ev(ev);
}


void in_dispatch_recorded_events()
{
	SDL_Event_ ev;

	while(state == PLAYBACK && next_ev_time <= game_ticks)
	{
		fread(&ev, sizeof(SDL_Event_), 1, f);

		// do this before dispatch_ev(),
		// in case a handler calls in_stop() (setting f to 0)
		if(!fread(&next_ev_time, sizeof(u32), 1, f))
{
			in_stop();
exit(0x73c07d);
// TODO: 'disconnect'?
}
		next_ev_time += time_adjust;			

		in_dispatch_event(&ev);	
	}
}
