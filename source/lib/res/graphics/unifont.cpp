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

#include "precompiled.h"
#include "unifont.h"

#include <stdio.h>
#include <string>
#include <sstream>
#include <map>

#include "ogl_tex.h"
#include "lib/res/h_mgr.h"

typedef std::map<u16, UnifontGlyphData> glyphmap;

struct UniFont
{
	Handle ht; // Handle to font texture

	bool HasRGB; // true if RGBA, false if ALPHA

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
static Status UniFont_reload(UniFont* f, const PIVFS& vfs, const VfsPath& basename, Handle UNUSED(h))
{
	// already loaded
	if(f->ht > 0)
		return INFO::OK;

	f->glyphs = new glyphmap();

	const VfsPath path(L"fonts/");

	// Read font definition file into a stringstream
	shared_ptr<u8> buf; size_t size;
	const VfsPath fntName(basename.ChangeExtension(L".fnt"));
	RETURN_STATUS_IF_ERR(vfs->LoadFile(path / fntName, buf, size));	// [cumulative for 12: 36ms]
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
		{
			f->HasRGB = true;
			fmt_ovr = GL_RGBA;
		}
		else if (Format == "a")
		{
			f->HasRGB = false;
			fmt_ovr = GL_ALPHA;
		}
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
			DEBUG_WARN_ERR(ERR::LOGIC);	// Invalid codepoint
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

		UnifontGlyphData g = { u, -v, u+w, -v+h, (i16)OffsetX, (i16)-OffsetY, (i16)(OffsetX+Width), (i16)(-OffsetY+Height), (i16)Advance };
		(*f->glyphs)[(u16)Codepoint] = g;
	}

	ENSURE(f->Height); // Ensure the height has been found (which should always happen if the font includes an 'I')

	// Load glyph texture
	// [cumulative for 12: 20ms]
	const VfsPath imgName(basename.ChangeExtension(L".png"));
	Handle ht = ogl_tex_load(vfs, path / imgName);
	RETURN_STATUS_IF_ERR(ht);
	(void)ogl_tex_set_filter(ht, GL_NEAREST);
	// override is necessary because the GL format is chosen as LUMINANCE,
	// but we want ALPHA. there is no way of knowing what format
	// 8bpp textures are in - we could adopt a naming convention and
	// add some TEX_ flags, but that's overkill.
	Status err = ogl_tex_upload(ht, fmt_ovr);
	if(err < 0)
	{
		(void)ogl_tex_free(ht);
		return err;
	}

	f->ht = ht;

	return INFO::OK;
}

static Status UniFont_validate(const UniFont* f)
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

static Status UniFont_to_string(const UniFont* f, wchar_t* buf)
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


Status unifont_unload(Handle& h)
{
	H_DEREF(h, UniFont, f);
	return h_free(h, H_UniFont);
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


bool unifont_has_rgb(const Handle h)
{
	UniFont* const f = H_USER_DATA(h, UniFont);
	if(!f)
		return false;
	return f->HasRGB;
}


int unifont_character_width(const Handle h, wchar_t c)
{
	H_DEREF(h, UniFont, f);
	glyphmap::iterator it = f->glyphs->find(c);
	
	if (it == f->glyphs->end())
		it = f->glyphs->find(0xFFFD); // Use the missing glyph symbol

	return it->second.xadvance;
}

Status unifont_stringsize(const Handle h, const wchar_t* text, int& width, int& height)
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
			DEBUG_WARN_ERR(ERR::LOGIC);	// Missing the missing glyph in a unifont!
			return INFO::OK;
		}

		width += it->second.xadvance; // Add the character's advance distance
	}

	return INFO::OK;
}

const glyphmap& unifont_get_glyphs(const Handle h)
{
	UniFont* const f = H_USER_DATA(h, UniFont);
	if(!f)
	{
		DEBUG_WARN_ERR(ERR::INVALID_HANDLE);
		static glyphmap dummy;
		return dummy;
	}

	return *f->glyphs;
}

Handle unifont_get_texture(const Handle h)
{
	UniFont* const f = H_USER_DATA(h, UniFont);
	if(!f)
	{
		DEBUG_WARN_ERR(ERR::INVALID_HANDLE);
		return 0;
	}

	return f->ht;
}
