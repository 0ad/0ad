/* Copyright (c) 2012 Wildfire Games
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
 * Unicode OpenGL texture font.
 */

#ifndef INCLUDED_UNIFONT
#define INCLUDED_UNIFONT

#include <stdarg.h>	// va_list
#include <map>

#include "lib/res/handle.h"
#include "lib/file/vfs/vfs.h"

/**
 * Load a font.
 *
 * @param vfs
 * @param pathname path and basename of the font definition file
 *		  (.fnt) and its texture (.png)
 * @param flags
 **/
extern Handle unifont_load(const PIVFS& vfs, const VfsPath& pathname, size_t flags = 0);

/**
 * Release a handle to a previously loaded font
 * (subject to reference counting).
 **/
extern Status unifont_unload(Handle& h);

/**
 * Determine pixel extents of a string.
 *
 * @param h
 * @param text string in question.
 * @param width
 * @param height is roughly the pixel height of a capital letter, for use
 * when aligning text in an aesthetically pleasing way.
 *
 * note: This is intended for the GUI (hence Unicode).
 **/
Status unifont_stringsize(const Handle h, const wchar_t* text, int& width, int& height);

/**
 * @return whether the font is an RGBA texture, not an ALPHA texture.
 **/
bool unifont_has_rgb(const Handle h);

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

// Raw access to font data (since it's convenient to move as much of the
// processing as possible to outside lib/):

struct UnifontGlyphData
{
	float u0, v0, u1, v1;
	i16 x0, y0, x1, y1;
	i16 xadvance;
};

/**
 * @return glyph data for all glyphs in this font.
 */
const std::map<u16, UnifontGlyphData>& unifont_get_glyphs(const Handle h);

/**
 * @return texture handle for this font.
 */
Handle unifont_get_texture(const Handle h);

#endif // INCLUDED_UNIFONT
