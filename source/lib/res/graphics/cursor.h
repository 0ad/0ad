/**
 * =========================================================================
 * File        : cursor.h
 * Project     : 0 A.D.
 * Description : mouse cursors (either via OpenGL texture or hardware)
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2004 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef CURSOR_H__
#define CURSOR_H__

// draw the specified cursor at the given pixel coordinates
// (origin is top-left to match the windowing system).
// uses a hardware mouse cursor where available, otherwise a
// portable OpenGL implementation.
extern LibError cursor_draw(const char* name, int x, int y);

// internal use only:
extern int g_yres;

#endif	// #ifndef CURSOR_H__
