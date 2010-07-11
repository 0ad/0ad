/* Copyright (c) 2010 Wildfire Games
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

#include "precompiled.h"
#include "unifont.h"

#include <stdio.h>
#include <string>
#include <sstream>
#include <map>

#include "ogl_tex.h"
#include "lib/res/h_mgr.h"

struct GlyphData
{
	float u0, v0, u1, v1;
	i16 x0, y0, x1, y1;
	i16 xadvance;
};

typedef std::map<u16, GlyphData> glyphmap;

static glyphmap* BoundGlyphs = NULL;

struct UniFont
{
	Handle ht; // Handle to font texture

	glyphmap* glyphs;

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

	SAFE_DELETE(f->glyphs);
}

// basename is e.g. "console"; the files are "fonts/console.fnt" and "fonts/console.png"
// [10..70ms]
static LibError UniFont_reload(UniFont* f, const PIVFS& vfs, const VfsPath& basename, Handle UNUSED(h))
{
	// already loaded
	if(f->ht > 0)
		return INFO::OK;

	f->glyphs = new glyphmap();

	const VfsPath path(L"fonts/");

	// Read font definition file into a stringstream
	shared_ptr<u8> buf; size_t size;
	const VfsPath fntName(basename.string() + L".fnt");
	RETURN_ERR(vfs->LoadFile(path/fntName, buf, size));	// [cumulative for 12: 36ms]
	std::istringstream FNTStream(std::string((const char*)buf.get(), size));

	int Version;
	FNTStream >> Version;
	if (Version < 100 || Version > 101) // Make sure this is from a recent version of the font builder
		WARN_RETURN(ERR::FAIL);

	int TextureWidth, TextureHeight;
	FNTStream >> TextureWidth >> TextureHeight;

	GLenum fmt_ovr = GL_ALPHA;
	if (Version >= 101)
	{
		std::string Format;
		FNTStream >> Format;
		if (Format == "rgba")
			fmt_ovr = GL_RGBA;
		else if (Format == "a")
			fmt_ovr = GL_ALPHA;
		else
			debug_warn(L"Invalid .fnt format string");
	}

	int NumGlyphs;
	FNTStream >> NumGlyphs;

	FNTStream >> f->LineSpacing;

	if (Version >= 101)
		FNTStream >> f->Height;
	else
		f->Height = 0;

	// [cumulative for 12: 256ms]
	for (int i = 0; i < NumGlyphs; ++i)
	{
		int          Codepoint, TextureX, TextureY, Width, Height, OffsetX, OffsetY, Advance;
		FNTStream >> Codepoint>>TextureX>>TextureY>>Width>>Height>>OffsetX>>OffsetY>>Advance;

		if (Codepoint < 0 || Codepoint > 0xFFFF)
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

		GlyphData g = { u, -v, u+w, -v+h, (i16)OffsetX, (i16)-OffsetY, (i16)(OffsetX+Width), (i16)(-OffsetY+Height), (i16)Advance };
		(*f->glyphs)[(u16)Codepoint] = g;
	}

	debug_assert(f->Height); // Ensure the height has been found (which should always happen if the font includes an 'I')

	// Load glyph texture
	// [cumulative for 12: 20ms]
	const VfsPath imgName(basename.string() + L".png");
	Handle ht = ogl_tex_load(vfs, path/imgName);
	RETURN_ERR(ht);
	(void)ogl_tex_set_filter(ht, GL_NEAREST);
	// override is necessary because the GL format is chosen as LUMINANCE,
	// but we want ALPHA. there is no way of knowing what format
	// 8bpp textures are in - we could adopt a naming convention and
	// add some TEX_ flags, but that's overkill.
	LibError err = ogl_tex_upload(ht, fmt_ovr);
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
	if(debug_IsPointerBogus(f->glyphs))
		WARN_RETURN(ERR::_2);
	// <LineSpacing> and <Height> are read directly from font file.
	// negative values don't make sense, but that's all we can check.
	if(f->LineSpacing < 0 || f->Height < 0)
		WARN_RETURN(ERR::_3);
	return INFO::OK;
}

static LibError UniFont_to_string(const UniFont* f, wchar_t* buf)
{
	if (f->ht) // not true if this is called after dtor (which it is)
	{
		const VfsPath& path = h_filename(f->ht);
		swprintf_s(buf, H_STRING_LEN, L"Font %ls", path.string().c_str());
	}
	else
		swprintf_s(buf, H_STRING_LEN, L"Font");
	return INFO::OK;
}


Handle unifont_load(const PIVFS& vfs, const VfsPath& pathname, size_t flags)
{
	return h_alloc(H_UniFont, vfs, pathname, flags);
}


LibError unifont_unload(Handle& h)
{
	H_DEREF(h, UniFont, f);

	// unbind ourself, so people will get errors if
	// they draw more text without binding a new font
	if (BoundGlyphs == f->glyphs)
		BoundGlyphs = NULL;

	return h_free(h, H_UniFont);
}


LibError unifont_bind(const Handle h)
{
	H_DEREF(h, UniFont, f);

	ogl_tex_bind(f->ht);
	BoundGlyphs = f->glyphs;

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
	glyphmap::iterator it = f->glyphs->find(c);
	
	if (it == f->glyphs->end())
		it = f->glyphs->find(0xFFFD); // Use the missing glyph symbol

	return it->second.xadvance;
}


struct t2f_v2i
{
	float u, v;
	i16 x, y;
};

void glvwprintf(const wchar_t* fmt, va_list args)
{
	const int buf_size = 1024;
	wchar_t buf[buf_size];

	int ret = vswprintf(buf, buf_size-1, fmt, args);
	if(ret < 0) {
		debug_printf(L"glwprintf failed (buffer size exceeded?) - return value %d, errno %d\n", ret, errno);
	}

	// Make sure there's always null termination
	buf[buf_size-1] = 0;

	debug_assert(BoundGlyphs != NULL); // You always need to bind something first

	// Count the number of characters
	size_t len = wcslen(buf);
	
	// 0 glyphs -> nothing to do (avoid BoundsChecker warning)
	if (!len)
		return;

	t2f_v2i* vertexes = new t2f_v2i[len*4];

	i32 x = 0;

	for (size_t i = 0; i < len; ++i)
	{
		glyphmap::iterator it = BoundGlyphs->find(buf[i]);

		if (it == BoundGlyphs->end())
			it = BoundGlyphs->find(0xFFFD); // Use the missing glyph symbol

		if (it == BoundGlyphs->end()) // Missing the missing glyph symbol - give up
			continue;

		const GlyphData& g = it->second;

		vertexes[i*4].u = g.u0;
		vertexes[i*4].v = g.v0;
		vertexes[i*4].x = g.x0 + x;
		vertexes[i*4].y = g.y0;

		vertexes[i*4+1].u = g.u1;
		vertexes[i*4+1].v = g.v0;
		vertexes[i*4+1].x = g.x1 + x;
		vertexes[i*4+1].y = g.y0;

		vertexes[i*4+2].u = g.u1;
		vertexes[i*4+2].v = g.v1;
		vertexes[i*4+2].x = g.x1 + x;
		vertexes[i*4+2].y = g.y1;

		vertexes[i*4+3].u = g.u0;
		vertexes[i*4+3].v = g.v1;
		vertexes[i*4+3].x = g.x0 + x;
		vertexes[i*4+3].y = g.y1;

		x += g.xadvance;
	}

	ogl_WarnIfError();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(2, GL_SHORT, sizeof(t2f_v2i), (u8*)vertexes + offsetof(t2f_v2i, x));
	glTexCoordPointer(2, GL_FLOAT, sizeof(t2f_v2i), (u8*)vertexes + offsetof(t2f_v2i, u));

	glDrawArrays(GL_QUADS, 0, (GLsizei)len*4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	ogl_WarnIfError();

	// Move into position for subsequent prints
	glTranslatef((float)x, 0, 0);

	delete[] vertexes;
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
		glyphmap::iterator it = f->glyphs->find(text[i]);

		if (it == f->glyphs->end())
			it = f->glyphs->find(0xFFFD); // Use the missing glyph symbol

		if (it == f->glyphs->end()) // Missing the missing glyph symbol - give up
		{
			debug_assert(0);	// Missing the missing glyph in a unifont!
			return INFO::OK;
		}

		width += it->second.xadvance; // Add the character's advance distance
	}

	return INFO::OK;
}
