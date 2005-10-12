/*
$Id$

Unicode OpenGL texture font
  
   -- Philip Taylor (philip@wildfiregames.com / philip@zaynar.demon.co.uk)
      (based on Jan Wassenberg's font.cpp/.h)
*/

#include "precompiled.h"

#include "lib.h"
#include "../res.h"
#include "ogl.h"
#include "ogl_tex.h"

#include <string>
#include <sstream>
#include <map>

#include <stdio.h>

// This isn't particularly efficient - it can be improved if we
// (a) care enough, and (b) know about fixed ranges of characters
// that the fonts usually contain
typedef std::map<wchar_t, unsigned short> glyphmap_id;
// Store the size separately, because it's used separately
typedef std::map<wchar_t, int> glyphmap_size;

glyphmap_id* BoundGlyphs = NULL;

struct UniFont
{
	Handle ht; // Handle to font texture

	glyphmap_id* glyphs_id; // Stored as pointers to keep the struct's size down. (sizeof(std::map)==12, though that's probably not enough to matter)
	glyphmap_size* glyphs_size;

	uint ListBase;
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

static int UniFont_reload(UniFont* f, const char* fn, Handle UNUSED(h))
{
	// already loaded
	if(f->ht > 0)
		return 0;

	f->glyphs_id = new glyphmap_id;
	f->glyphs_size = new glyphmap_size;

	// fn is the base filename, e.g. "console"
	// The font definition file is "fonts/"+fn+".fnt" and the texture is "fonts/"+fn+".tga"
	std::string FilenameBase = "fonts/"; FilenameBase += fn;

	// Read font definition file into a stringstream
	void* RawFNT;
	size_t FNTSize;
	std::string FilenameFnt = FilenameBase+".fnt";
	const char* fnt_fn = FilenameFnt.c_str();
	Handle hm = vfs_load(fnt_fn, RawFNT, FNTSize);
	RETURN_ERR(hm);
	std::istringstream FNTStream (std::string((const char*)RawFNT, (int)FNTSize));
	mem_free_h(hm);

	int Version;
	FNTStream >> Version;
	if (Version != 100) // Make sure this is from a recent version of the font builder
	{
		debug_warn("Invalid .fnt version number");
		return ERR_UNKNOWN_FORMAT;
	}

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
	{
		debug_warn("Display list creation failed");
		return -1;
	}

	for (int i = 0; i < NumGlyphs; ++i)
	{
		int          Codepoint, TextureX, TextureY, Width, Height, OffsetX, OffsetY, Advance;
		FNTStream >> Codepoint>>TextureX>>TextureY>>Width>>Height>>OffsetX>>OffsetY>>Advance;

		if (Codepoint > 0xffff)
		{
			debug_warn("Invalid codepoint");
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

		glNewList(f->ListBase+i, GL_COMPILE);
			glBegin(GL_QUADS);
				glTexCoord2f(u,   -v);   glVertex2i(OffsetX,       -OffsetY);
				glTexCoord2f(u+w, -v);   glVertex2i(OffsetX+Width, -OffsetY);
				glTexCoord2f(u+w, -v+h); glVertex2i(OffsetX+Width, -OffsetY+Height);
				glTexCoord2f(u,   -v+h); glVertex2i(OffsetX,       -OffsetY+Height);
			glEnd();
			glTranslatef((GLfloat)Advance, 0, 0);
		glEndList();

		(*f->glyphs_id)[(wchar_t)Codepoint] = (unsigned short)i;
		(*f->glyphs_size)[(wchar_t)Codepoint] = Advance;
	}

	debug_assert(f->Height); // Ensure the height has been found (which should always happen if the font includes an 'I')

	// Load glyph texture
	std::string FilenameTex = FilenameBase+".tga";  
	const char* tex_fn = FilenameTex.c_str();   
	Handle ht = ogl_tex_load(tex_fn);
	RETURN_ERR(ht);

	(void)ogl_tex_set_filter(ht, GL_NEAREST);

	// override is necessary because the GL format is chosen as LUMINANCE,
	// but we want ALPHA. there is no way of knowing what format
	// 8bpp textures are in - we could adopt a naming convention and
	// add some TEX_ flags, but that's overkill.
	int err = ogl_tex_upload(ht, GL_ALPHA);
	if(err < 0)
	{
		(void)ogl_tex_free(ht);
		return err;
	}

	f->ht = ht;
	return 0;
}

static int UniFont_validate(const UniFont* f)
{
	if(f->ht < 0)
		return -2;
	if(debug_is_pointer_bogus(f->glyphs_id) || debug_is_pointer_bogus(f->glyphs_size))
		return -3;
	// <LineSpacing> and <Height> are read directly from font file.
	// negative values don't make sense, but that's all we can check.
	if(f->LineSpacing < 0 || f->Height < 0)
		return -4;
	if(f->ListBase == 0 || f->ListBase > 1000000)	// suspicious
		return -5;
	return 0;
}


Handle unifont_load(const char* fn, int scope)
{
	return h_alloc(H_UniFont, fn, scope);
}


int unifont_unload(Handle& h)
{
	return h_free(h, H_UniFont);
}


int unifont_bind(const Handle h)
{
	H_DEREF(h, UniFont, f);

	ogl_tex_bind(f->ht);
	glListBase(f->ListBase);
	BoundGlyphs = f->glyphs_id;

	return 0;
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


int unifont_character_width(const Handle h, const wchar_t& c)
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

	if(vswprintf(buf, buf_size-1, fmt, args) < 0)
		debug_printf("glwprintf failed (buffer size exceeded?)\n");

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
			debug_warn("Missing the missing glyph in a unifont!\n");
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


int unifont_stringsize(const Handle h, const wchar_t* text, int& width, int& height)
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
			debug_warn("Missing the missing glyph in a unifont!\n");
			return 0;
		}

		width += it->second; // Add the character's advance distance
	}

	return 0;
}
