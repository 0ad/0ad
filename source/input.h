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

#ifndef __INPUT_H__
#define __INPUT_H__


#include "wsdl.h"
#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif


extern u32 game_ticks;


typedef bool (*IN_HANDLER)(const SDL_Event& event);

/*
 * register an input handler, which will receive all subsequent events first.
 * events are passed to other handlers if handler returns false.
 */
extern int in_add_handler(IN_HANDLER handler);

extern void in_get_events();

extern int in_record(const char* fn);
extern int in_playback(const char* fn);
extern void in_stop();


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef __INPUT_H__ */
