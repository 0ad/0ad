/*
$Id: unifont.cpp,v 1.12 2004/07/31 01:57:26 janwas Exp $

Unicode OpenGL texture font
  
   -- Philip Taylor (philip@wildfiregames.com / philip@zaynar.demon.co.uk)
      (based on Jan Wassenberg's font.cpp/.h)
*/

#include "precompiled.h"

#include "lib.h"
#include "res.h"
#include "ogl.h"

#include <string>
#include <sstream>
#include <map>

#include <stdio.h>
#include <assert.h>

// This isn't particularly efficient - it can be improved if we
// (a) care enough, and (b) know about fixed ranges of characters
// that the fonts usually contain
typedef std::map<wchar_t, unsigned short> glyphmap_id;
// Store the size separately, because it's used separately
typedef std::map<wchar_t, int> glyphmap_size;

glyphmap_id* BoundGlyphs = NULL;

struct UniFont
{
	Handle ht; // handle to font texture
	glyphmap_id* glyphs_id;
	glyphmap_size* glyphs_size;
	uint ListBase;
	int LineSpacing;
};

H_TYPE_DEFINE(UniFont);

static void UniFont_init(UniFont* f, va_list UNUSEDPARAM(args))
{
	f->glyphs_id = new glyphmap_id;
	f->glyphs_size = new glyphmap_size;
}

static void UniFont_dtor(UniFont* f)
{
	tex_free(f->ht);
	glDeleteLists(f->ListBase, (GLsizei)f->glyphs_id->size());
	delete f->glyphs_id;
	delete f->glyphs_size;
}


static int UniFont_reload(UniFont* f, const char* fn, Handle UNUSEDPARAM(h))
{
	// fn is the base filename, like "console"
	// The font definition file is "fonts/"+fn+".fnt" and the texture is "fonts/"+fn+".tga"

	std::string FilenameBase = "fonts/"; FilenameBase += fn;

	void* RawFNT;
	size_t FNTSize;
	std::string FilenameFnt = FilenameBase+".fnt";  
	const char* fnt_fn = FilenameFnt.c_str();   
	Handle fh = vfs_load(fnt_fn, RawFNT, FNTSize);
	CHECK_ERR(fh);

	// Get the data in a nicer object
	std::istringstream FNTStream (std::string((char*)RawFNT, (int)FNTSize));

	// Unload the file
	mem_free(RawFNT);

	int Version;
	FNTStream >> Version;
	assert (Version == 100); // Make sure this is from a recent version of the font builder

	int TextureWidth, TextureHeight;
	FNTStream >> TextureWidth >> TextureHeight;

	int NumGlyphs;
	FNTStream >> NumGlyphs;

	FNTStream >> f->LineSpacing;

	f->ListBase = glGenLists(NumGlyphs);
	assert(f->ListBase != 0); // My Voodoo2 drivers didn't support display lists (although I'd be surprised if they got this far)

	for (int i = 0; i < NumGlyphs; ++i)
	{
		int Codepoint, TextureX, TextureY, Width, Height, OffsetX, OffsetY, Advance;
		FNTStream >> Codepoint >> TextureX >> TextureY >> Width >> Height >> OffsetX >> OffsetY >> Advance;

		assert(Codepoint <= 0xffff);

		GLfloat u = (GLfloat)TextureX / (GLfloat)TextureWidth;
		GLfloat v = (GLfloat)TextureY / (GLfloat)TextureHeight;
		GLfloat w = (GLfloat)Width  / (GLfloat)TextureWidth;
		GLfloat h = (GLfloat)Height / (GLfloat)TextureHeight;

		glNewList(f->ListBase+i, GL_COMPILE);
			glBegin(GL_QUADS);
				glTexCoord2f(u,   v);	glVertex2i(OffsetX,       -OffsetY);
				glTexCoord2f(u+w, v);	glVertex2i(OffsetX+Width, -OffsetY);
				glTexCoord2f(u+w, v-h);	glVertex2i(OffsetX+Width, -OffsetY+Height);
				glTexCoord2f(u,   v-h);	glVertex2i(OffsetX,       -OffsetY+Height);
			glEnd();
			glTranslatef((GLfloat)Advance, 0, 0);
		glEndList();

		(*f->glyphs_id)[(wchar_t)Codepoint] = (unsigned short)i;
		(*f->glyphs_size)[(wchar_t)Codepoint] = Advance;
	}

	// Load glyph texture
	std::string FilenameTex = FilenameBase+".tga";  
	const char* tex_fn = FilenameTex.c_str();   
	const Handle ht = tex_load(tex_fn);
	if (ht <= 0)
		return (int)ht;

	tex_upload(ht, GL_NEAREST, GL_ALPHA8, GL_ALPHA);

	f->ht = ht;

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

	tex_bind(f->ht);
	glListBase(f->ListBase);
	BoundGlyphs = f->glyphs_id;

	return 0;
}


int unifont_linespacing(const Handle h)
{
	H_DEREF(h, UniFont, f);
	return f->LineSpacing;
}


void glwprintf(const wchar_t* fmt, ...)
{
	va_list args;
	const int buf_size = 1024;
	wchar_t buf[buf_size];

	va_start(args, fmt);
	if (vswprintf(buf, buf_size-1, fmt, args) < 0)
		debug_out("glwprintf failed (buffer size exceeded?)\n");
	va_end(args);

	// Make sure there's always NULL termination
	buf[buf_size-1] = 0;

	assert(BoundGlyphs != NULL); // You always need to bind something first

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
			debug_out("Missing the missing glyph in a unifont!\n");
			return;
		}

		buf[i] = (*it).second; // Replace with the display list offset
	}

	// 0 gylphs -> nothing to do (avoid BoundsChecker warning)
	if(!len)
		return;

	// Execute all the display lists
	glCallLists((GLsizei)len, sizeof(wchar_t)==4?GL_INT:GL_UNSIGNED_SHORT, buf);
}


int unifont_stringsize(const Handle h, const char* text, int& width, int& height)
{
	H_DEREF(h, UniFont, f);

	width = 0;
	height = f->LineSpacing;

	size_t len = strlen(text);

	for (size_t i = 0; i < len; ++i)
	{
		glyphmap_size::iterator it = f->glyphs_size->find((wchar_t)text[i]);

		if (it == f->glyphs_size->end())
			it = f->glyphs_size->find(0xFFFD); // Use the missing glyph symbol

		if (it == f->glyphs_size->end()) // Missing the missing glyph symbol - give up
		{
			debug_out("Missing the missing glyph in a unifont!\n");
			return 0;
		}

		width += (*it).second; // Add the character's advance distance
	}

	return 0;
}
