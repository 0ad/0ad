/**
 * =========================================================================
 * File        : unifont.h
 * Project     : 0 A.D.
 * Description : Unicode OpenGL texture font.
 * =========================================================================
 */

#ifndef INCLUDED_UNIFONT
#define INCLUDED_UNIFONT

#include <stdarg.h>	// va_list

#include "lib/res/handle.h"

// Load and return a handle to the font defined
// in fn+".fnt" with texture fn+".tga"
extern Handle unifont_load(const VfsPath& pathname, int flags = 0);

// Release a handle to a previously loaded font
extern LibError unifont_unload(Handle& h);

// Use the font referenced by h for all subsequent glwprintf() calls.
// Must be called before any glwprintf().
extern LibError unifont_bind(Handle h);

// Output text at current OpenGL modelview pos.
extern void glvwprintf(const wchar_t* fmt, va_list args);
extern void glwprintf(const wchar_t* fmt, ...);

/*
  glwprintf assumes an environment roughly like:

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
*/

// Intended for the GUI (hence Unicode). 'height' is roughly the height of
// a capital letter, for use when aligning text in an aesthetically pleasing way.
LibError unifont_stringsize(const Handle h, const wchar_t* text, int& width, int& height);

// Get only the height
int unifont_height(const Handle h);

// Get only the width of one character
int unifont_character_width(const Handle h, wchar_t c);

// Return spacing in pixels from one line of text to the next
int unifont_linespacing(const Handle h);

#endif // INCLUDED_UNIFONT
