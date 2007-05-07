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

// draw the specified cursor at the given pixel coordinates
// (origin is top-left to match the windowing system).
// uses a hardware mouse cursor where available, otherwise a
// portable OpenGL implementation.
extern LibError cursor_draw(const char* name, int x, int y);

// internal use only:
extern int g_yres;

#endif	// #ifndef INCLUDED_GRAPHICS_CURSOR
