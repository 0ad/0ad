// $Id: unifont.h,v 1.1 2004/06/16 15:36:28 philip Exp $

#ifndef __UNIFONT_H__
#define __UNIFONT_H__

//#include "types.h"
#include "h_mgr.h"


// Load and return a handle to the font defined
// in fn+".fnt" with texture fn+".tga"
extern Handle unifont_load(const char* fn, int scope = RES_STATIC);

// Use the font referenced by h for all subsequent glwprintf() calls.
// Must be called before any glwprintf().
extern int unifont_bind(Handle h);

// Output text at current OpenGL modelview pos.
extern void glwprintf(const wchar_t* fmt, ...);

/*
  glwprintf assumes an environment rougly like:

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
*/

// Return spacing in pixels from one line of text to the next
int unifont_linespacing(const Handle h);

#endif // __UNIFONT_H__
