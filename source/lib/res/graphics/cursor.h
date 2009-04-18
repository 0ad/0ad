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

/**
 * =========================================================================
 * File        : cursor.h
 * Project     : 0 A.D.
 * Description : mouse cursors (either via OpenGL texture or hardware)
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_GRAPHICS_CURSOR
#define INCLUDED_GRAPHICS_CURSOR

/**
 * draw the cursor on-screen.
 *
 * @param name base name of cursor or zero to hide the cursor.
 * @param x,y coordinates [pixels] (origin is in the upper left)
 *
 * uses a hardware mouse cursor where available, otherwise a
 * portable OpenGL implementation.
 **/
extern LibError cursor_draw(const char* name, int x, int y);

// internal use only:
extern int g_yres;

#endif	// #ifndef INCLUDED_GRAPHICS_CURSOR
