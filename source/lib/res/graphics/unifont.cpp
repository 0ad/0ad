/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : unifont.cpp
 * Project     : 0 A.D.
 * Description : Unicode OpenGL texture font.
 * =========================================================================
 */

#include "precompiled.h"
#include "unifont.h"

#include <stdio.h>
#include <string>
#include <sstream>
#include <map>

#include "ogl_tex.h"
#include "../h_mgr.h"
#include "lib/file/vfs/vfs.h"
extern PIVFS g_VFS;

// This isn't particularly efficient - it can be improved if we
// (a) care enough, and (b) know about fixed ranges of characters
// that the fonts usually contain
typedef std::map<wchar_t, unsigned short> glyphmap_id;
// Store the size separately, because it's used separately
typedef std::map<wchar_t, int> glyphmap_size;

static glyphmap_id* BoundGlyphs = NULL;

struct UniFont
{
	Handle ht; // Handle to font texture

	glyphmap_id* glyphs_id; // Stored as pointers to keep the struct's size down. (sizeof(std::map)==12, though that's probably not enough to matter)
	glyphmap_size* glyphs_size;

	GLuint ListBase;
	int LineSpacing;
	int Height; // of a capital letter, roughly
};

H_TYPE_DEFINE(UniFont);

static void UniFont_init(UniFont* UNUSED(f), va_list UNUSED(args))
{
}

static void UniFont_dtor(UniFont* f)
{
	// these are all safe, no is_valid flags needed
	(void)ogl_tex_free(f->ht);

	glDeleteLists(f->ListBase, (GLsizei)f->glyphs_id->size());
	f->ListBase = 0;

	SAFE_DELETE(f->glyphs_id);
	SAFE_DELETE(f->glyphs_size);
}

// basename is e.g. "console"; the files are "fonts/console.fnt" and "fonts/console.tga"
// [10..70ms]
static LibError UniFont_reload(UniFont* f, const VfsPath& basename, Handle UNUSED(h))
{
	// already loaded
	if(f->ht > 0)
		return INFO::OK;

	f->glyphs_id = new glyphmap_id();
	f->glyphs_size = new glyphmap_size();

	const VfsPath path("fonts/");

	// Read font definition file into a stringstream
	shared_ptr<u8> buf; size_t size;
	const VfsPath fntName(basename.string() + ".fnt");
	RETURN_ERR(g_VFS->LoadFile(path/fntName, buf, size));	// [cumulative for 12: 36ms]
	std::istringstream FNTStream (std::string((const char*)buf.get(), size));

	int Version;
	FNTStream >> Version;
	if (Version != 100) // Make sure this is from a recent version of the font builder
		WARN_RETURN(ERR::FAIL);

	int TextureWidth, TextureHeight;
	FNTStream >> TextureWidth >> TextureHeight;

	int NumGlyphs;
	FNTStream >> NumGlyphs;

	FNTStream >> f->LineSpacing;

	if (Version >= 101)
		FNTStream >> f->Height;
	else
		f->Height = 0;

	f->ListBase = glGenLists(NumGlyphs);
	if (f->ListBase == 0) // My Voodoo2 drivers didn't support display lists (although I'd be surprised if they got this far)
		WARN_RETURN(ERR::FAIL);

	// [cumulative for 12: 256ms]
	for (int i = 0; i < NumGlyphs; ++i)
	{
		int          Codepoint, TextureX, TextureY, Width, Height, OffsetX, OffsetY, Advance;
		FNTStream >> Codepoint>>TextureX>>TextureY>>Width>>Height>>OffsetX>>OffsetY>>Advance;

		if (Codepoint > 0xFFFF)
		{
			debug_assert(0);	// Invalid codepoint
			continue;
		}

		if (Version < 101 && Codepoint == 'I')
		{
			f->Height = Height;
		}

		GLfloat u = (GLfloat)TextureX / (GLfloat)TextureWidth;
		GLfloat v = (GLfloat)TextureY / (GLfloat)TextureHeight;
		GLfloat w = (GLfloat)Width  / (GLfloat)TextureWidth;
		GLfloat h = (GLfloat)Height / (GLfloat)TextureHeight;

		// It might be better to use vertex arrays instead of display lists,
		// but this works well enough for now.
		// [cumulative for 12: 180ms]
		glNewList(f->ListBase+i, GL_COMPILE);
			glBegin(GL_QUADS);
				glTexCoord2f(u,   -v);   glVertex2i(OffsetX,       -OffsetY);
				glTexCoord2f(u+w, -v);   glVertex2i(OffsetX+Width, -OffsetY);
				glTexCoord2f(u+w, -v+h); glVertex2i(OffsetX+Width, -OffsetY+Height);
				glTexCoord2f(u,   -v+h); glVertex2i(OffsetX,       -OffsetY+Height);
			glEnd();
			glTranslatef((GLfloat)Advance, 0, 0);
		glEndList();

		// [cumulative for 12: 20ms]
		(*f->glyphs_id)[(wchar_t)Codepoint] = (unsigned short)i;
		(*f->glyphs_size)[(wchar_t)Codepoint] = Advance;
	}

	debug_assert(f->Height); // Ensure the height has been found (which should always happen if the font includes an 'I')

	// Load glyph texture
	// [cumulative for 12: 20ms]
	const VfsPath tgaName(basename.string() + ".tga");
	Handle ht = ogl_tex_load(path/tgaName);
	RETURN_ERR(ht);
	(void)ogl_tex_set_filter(ht, GL_NEAREST);
	// override is necessary because the GL format is chosen as LUMINANCE,
	// but we want ALPHA. there is no way of knowing what format
	// 8bpp textures are in - we could adopt a naming convention and
	// add some TEX_ flags, but that's overkill.
	LibError err = ogl_tex_upload(ht, GL_ALPHA);
	if(err < 0)
	{
		(void)ogl_tex_free(ht);
		return err;
	}

	f->ht = ht;

	return INFO::OK;
}

static LibError UniFont_validate(const UniFont* f)
{
	if(f->ht < 0)
		WARN_RETURN(ERR::_1);
	if(debug_IsPointerBogus(f->glyphs_id) || debug_IsPointerBogus(f->glyphs_size))
		WARN_RETURN(ERR::_2);
	// <LineSpacing> and <Height> are read directly from font file.
	// negative values don't make sense, but that's all we can check.
	if(f->LineSpacing < 0 || f->Height < 0)
		WARN_RETURN(ERR::_3);
	if(f->ListBase == 0 || f->ListBase > 1000000)	// suspicious
		WARN_RETURN(ERR::_4);
	return INFO::OK;
}

static LibError UniFont_to_string(const UniFont* f, char* buf)
{
	if (f->ht) // not true if this is called after dtor (which it is)
	{
		const VfsPath& path = h_filename(f->ht);
		snprintf(buf, H_STRING_LEN, "Font %s", path.string().c_str());
	}
	else
		snprintf(buf, H_STRING_LEN, "Font");
	return INFO::OK;
}


Handle unifont_load(const VfsPath& pathname, size_t flags)
{
	return h_alloc(H_UniFont, pathname, flags);
}


LibError unifont_unload(Handle& h)
{
	return h_free(h, H_UniFont);
}


LibError unifont_bind(const Handle h)
{
	H_DEREF(h, UniFont, f);

	ogl_tex_bind(f->ht);
	glListBase(f->ListBase);
	BoundGlyphs = f->glyphs_id;

	return INFO::OK;
}


int unifont_linespacing(const Handle h)
{
	H_DEREF(h, UniFont, f);
	return f->LineSpacing;
}


int unifont_height(const Handle h)
{
	H_DEREF(h, UniFont, f);
	return f->Height;
}


int unifont_character_width(const Handle h, wchar_t c)
{
	H_DEREF(h, UniFont, f);
	glyphmap_size::iterator it = f->glyphs_size->find(c);
	
	if (it == f->glyphs_size->end())
		it = f->glyphs_size->find(0xFFFD); // Use the missing glyph symbol

	return it->second;
}


void glvwprintf(const wchar_t* fmt, va_list args)
{
	const int buf_size = 1024;
	wchar_t buf[buf_size];

	int ret = vswprintf(buf, buf_size-1, fmt, args);
	if(ret < 0) {
		debug_printf("glwprintf failed (buffer size exceeded?) - return value %d, errno %d\n", ret, errno);
	}

	// Make sure there's always null termination
	buf[buf_size-1] = 0;

	debug_assert(BoundGlyphs != NULL); // You always need to bind something first

	// Count the number of characters
	size_t len = wcslen(buf);
	
	// Replace every wchar_t with the display list offset for the appropriate glyph
	for (size_t i = 0; i < len; ++i)
	{
		glyphmap_id::iterator it = BoundGlyphs->find(buf[i]);

		if (it == BoundGlyphs->end())
			it = BoundGlyphs->find(0xFFFD); // Use the missing glyph symbol
		
		if (it == BoundGlyphs->end()) // Missing the missing glyph symbol - give up
		{
			debug_assert(0);	// Missing the missing glyph in a unifont!
			return;
		}

		buf[i] = it->second; // Replace with the display list offset
	}

	// 0 glyphs -> nothing to do (avoid BoundsChecker warning)
	if (!len)
		return;

	// Execute all the display lists
	glCallLists((GLsizei)len, sizeof(wchar_t)==4 ? GL_INT : GL_UNSIGNED_SHORT, buf);
}


void glwprintf(const wchar_t* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	glvwprintf(fmt, args);
	va_end(args);
}


LibError unifont_stringsize(const Handle h, const wchar_t* text, int& width, int& height)
{
	H_DEREF(h, UniFont, f);

	width = 0;
	height = f->Height;

	size_t len = wcslen(text);

	for (size_t i = 0; i < len; ++i)
	{
		glyphmap_size::iterator it = f->glyphs_size->find(text[i]);

		if (it == f->glyphs_size->end())
			it = f->glyphs_size->find(0xFFFD); // Use the missing glyph symbol

		if (it == f->glyphs_size->end()) // Missing the missing glyph symbol - give up
		{
			debug_assert(0);	// Missing the missing glyph in a unifont!
			return INFO::OK;
		}

		width += it->second; // Add the character's advance distance
	}

	return INFO::OK;
}
