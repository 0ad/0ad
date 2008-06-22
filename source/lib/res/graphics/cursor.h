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
