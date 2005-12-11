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
