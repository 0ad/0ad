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

#include "precompiled.h"

#include <unistd.h>
#include <stdio.h>
#include <wchar.h>

#include "lib/external_libraries/sdl.h"
#include "lib/wchar.h"
#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/cursor.h"
#include "udbg.h"

#define GNU_SOURCE
#include <dlfcn.h>


// these are basic POSIX-compatible backends for the sysdep.h functions.
// Win32 has better versions which override these.

void sys_display_msg(const wchar_t* caption, const wchar_t* msg)
{
	fwprintf(stderr, L"%ls: %ls\n", caption, msg);
}

ErrorReaction sys_display_error(const wchar_t* text, size_t flags)
{
	printf("%ls\n\n", text);

	const bool manual_break   = (flags & DE_MANUAL_BREAK  ) != 0;
	const bool allow_suppress = (flags & DE_ALLOW_SUPPRESS) != 0;
	const bool no_continue    = (flags & DE_NO_CONTINUE   ) != 0;

	// until valid input given:
	for(;;)
	{
		if(!no_continue)
			printf("(C)ontinue, ");
		if(allow_suppress)
			printf("(S)uppress, ");
		printf("(B)reak, Launch (D)ebugger, or (E)xit?\n");
		// TODO Should have some kind of timeout here.. in case you're unable to
		// access the controlling terminal (As might be the case if launched
		// from an xterm and in full-screen mode)
		int c = getchar();
		// note: don't use tolower because it'll choke on EOF
		switch(c)
		{
		case EOF:
		case 'd': case 'D':
			udbg_launch_debugger();
			//-fallthrough

		case 'b': case 'B':
			if(manual_break)
				return ER_BREAK;
			debug_break();
			return ER_CONTINUE;

		case 'c': case 'C':
			if(!no_continue)
                return ER_CONTINUE;
			// continue isn't allowed, so this was invalid input. loop again.
			break;
		case 's': case 'S':
			if(allow_suppress)
				return ER_SUPPRESS;
			// suppress isn't allowed, so this was invalid input. loop again.
			break;

		case 'e': case 'E':
			abort();
			return ER_EXIT;	// placebo; never reached
		}
	}
}


LibError sys_error_description_r(int err, wchar_t* buf, size_t max_chars)
{
	UNUSED2(err);
	UNUSED2(buf);
	UNUSED2(max_chars);

	// don't need to do anything: lib/errors.cpp already queries
	// libc's strerror(). if we ever end up needing translation of
	// e.g. Qt or X errors, that'd go here.
	return ERR::FAIL;
}

// stub for sys_cursor_create - we don't need to implement this (SDL/X11 only
// has monochrome cursors so we need to use the software cursor anyways)

// note: do not return ERR_NOT_IMPLEMENTED or similar because that
// would result in WARN_ERRs.
LibError sys_cursor_create(size_t w, size_t h, void* bgra_img, size_t hx, size_t hy, sys_cursor* cursor)
{
	UNUSED2(w);
	UNUSED2(h);
	UNUSED2(hx);
	UNUSED2(hy);
	UNUSED2(bgra_img);

	*cursor = 0;
	return INFO::OK;
}

// creates an empty cursor
LibError sys_cursor_create_empty(sys_cursor* cursor)
{
	/* bitmap for a fully transparent cursor */
	u8 data[] = {0};
	u8 mask[] = {0};

	// size 8x1 (cursor size must be a whole number of bytes ^^)
	// hotspot at 0,0
	// SDL will make its own copies of data and mask
	*cursor = SDL_CreateCursor(data, mask, 8, 1, 0, 0);

	return cursor ? INFO::OK : ERR::FAIL;
}

SDL_Cursor *defaultCursor=NULL;
// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
LibError sys_cursor_set(sys_cursor cursor)
{
	// Gaah, SDL doesn't have a good API for setting the default cursor
	// SetCursor(NULL) just /repaints/ the cursor (well, obviously! or...)
	ONCE(defaultCursor = SDL_GetCursor());

	// restore default cursor.
	if(!cursor)
		SDL_SetCursor(defaultCursor);

	SDL_SetCursor((SDL_Cursor *)cursor);

	return INFO::OK;
}

// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
LibError sys_cursor_free(sys_cursor cursor)
{
	// bail now to prevent potential confusion below; there's nothing to do.
	if(!cursor)
		return INFO::OK;

	// if the cursor being freed is active, restore the default cursor
	// (just for safety).
	if (SDL_GetCursor() == (SDL_Cursor *)cursor)
		WARN_ERR(sys_cursor_set(NULL));

	SDL_FreeCursor((SDL_Cursor *)cursor);

	return INFO::OK;
}

int sys_wopen(const wchar_t* pathname, int oflag, ...)
{
	mode_t mode = 0;
	if(oflag & O_CREAT)
	{
		va_list args;
		va_start(args, oflag);
		mode = va_arg(args, mode_t);
		va_end(args);
	}

	return open(utf8_from_wstring(pathname).c_str(), oflag, mode);
}

// note: just use the sector size: Linux aio doesn't really care about
// the alignment of buffers/lengths/offsets, so we'll just pick a
// sane value and not bother scanning all drives.
size_t sys_max_sector_size()
{
	// users may call us more than once, so cache the results.
	static size_t cached_sector_size;
	if(!cached_sector_size)
		cached_sector_size = sysconf(_SC_PAGE_SIZE);
	return cached_sector_size;
}
