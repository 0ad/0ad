// OpenGL texture font
// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef __FONT_H__
#define __FONT_H__

#include "types.h"
#include "h_mgr.h"


// load and return a handle to the font defined in <fn>
extern Handle font_load(const char* fn, int scope = RES_STATIC);

// use the font referenced by h for all subsequent glprintf() calls
extern int font_bind(Handle h);

// output text at current OpenGL modelview pos.
// assumes ortho projection with texturing, alpha test, and blending enabled.
// must bind a font before calling!
extern void glprintf(const char* fmt, ...);


#endif // #ifndef __FONT_H__




/*
EXAMPLE:

#include "font.h"


Handle h;

void init()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5);

	h = font_load("font.fnt");
	if(!h)
		abort();
}


void render()
{
	font_bind(h);
	glprintf("string");
}

// FONT FILE FORMAT:
%s					// texture file name
%d %d				// width/height of glyphs in the texture
%d [...] %d			// advance width for chars 32..127

*/
