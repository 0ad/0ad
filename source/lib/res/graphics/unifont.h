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

/**
 * Load a font.
 *
 * @param pathname path and basename of the font definition file
 * (.fnt) and its texture (.tga)
 **/
extern Handle unifont_load(const VfsPath& pathname, int flags = 0);

/**
 * Release a handle to a previously loaded font
 * (subject to reference counting).
 **/
extern LibError unifont_unload(Handle& h);

/**
 * Use a font for all subsequent glwprintf() calls.
 *
 * Must be called before any glwprintf().
 **/
extern LibError unifont_bind(Handle h);

/**
 * Output text at current OpenGL modelview pos.
 *
 * @param fmt - see fprintf
 *
 * this assumes an environment roughly like:
 * glEnable(GL_TEXTURE_2D);
 * glDisable(GL_CULL_FACE);
 * glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 * glEnable(GL_BLEND);
 * glDisable(GL_ALPHA_TEST);
 * glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
 **/
extern void glwprintf(const wchar_t* fmt, ...);

/**
 * varargs version of glwprintf.
 *
 * @param fmt, args - see vfprintf
 **/
extern void glvwprintf(const wchar_t* fmt, va_list args);

/**
 * Determine pixel extents of a string.
 *
 * @param text string in question.
 * @param height is roughly the pixel height of a capital letter, for use
 * when aligning text in an aesthetically pleasing way.
 *
 * note: This is intended for the GUI (hence Unicode).
 **/
LibError unifont_stringsize(const Handle h, const wchar_t* text, int& width, int& height);

/**
 * @return height [pixels] of the font.
 **/
int unifont_height(const Handle h);

/**
 * @return width [pixels] of a certain character.
 **/
int unifont_character_width(const Handle h, wchar_t c);

/**
 * @return spacing in pixels from one line of text to the next.
 **/
int unifont_linespacing(const Handle h);

#endif // INCLUDED_UNIFONT
