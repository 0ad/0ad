/* Copyright (c) 2017 Wildfire Games
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
 * mouse cursors (either via OpenGL texture or hardware)
 */

#ifndef INCLUDED_GRAPHICS_CURSOR
#define INCLUDED_GRAPHICS_CURSOR

#include "lib/file/vfs/vfs.h"

/**
 * Draw the cursor on-screen.
 *
 * @param vfs
 * @param name Base name of cursor or zero to hide the cursor.
 * @param x,y Coordinates [pixels] (origin at lower left)
 *		  (the origin is convenient for drawing via OpenGL, but requires the
 *		  mouse Y coordinate to be subtracted from the client area height.
 *		  Making the caller responsible for this avoids a dependency on
 *		  the g_yres global variable.)
 * @param scale Scale factor for drawing size the cursor.
 * @param forceGL Require the OpenGL cursor implementation, not hardware cursor
 *
 * Uses a hardware mouse cursor where available, otherwise a
 * portable OpenGL implementation.
 **/
extern Status cursor_draw(const PIVFS& vfs, const wchar_t* name, int x, int y, double scale, bool forceGL);

/**
 * Forcibly frees all cursor handles.
 *
 * Currently used just prior to SDL shutdown.
 */
void cursor_shutdown();

#endif	// #ifndef INCLUDED_GRAPHICS_CURSOR
