/*
$Id: unifont.cpp,v 1.6 2004/07/12 20:08:34 philip Exp $

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
typedef std::map<wchar_t, unsigned short> glyphmap;

glyphmap* BoundGlyphs = NULL;

struct UniFont
{
	Handle ht; // handle to font texture
	glyphmap* glyphs;
	uint ListBase;
	int LineSpacing;
};

H_TYPE_DEFINE(UniFont);

static void UniFont_init(UniFont* f, va_list args)
{
	f->glyphs = new glyphmap;
}

static void UniFont_dtor(UniFont* f)
{
	tex_free(f->ht);
	glDeleteLists(f->ListBase, (GLsizei)f->glyphs->size());
	f->glyphs->clear();
	delete f->glyphs;
}


static int UniFont_reload(UniFont* f, const char* fn, Handle h)
{
	// fn is the base filename, like "fonts/console"
	// The font definition file is fn+".fnt" and the texture is fn+".tga"

	std::string FilenameBase = fn;

	void* RawFNT;
	size_t FNTSize;
	Handle err = vfs_load((FilenameBase+".fnt").c_str(), RawFNT, FNTSize);

	if (err <= 0)
		return (int)err;

	// Get the data in a nicer object
	std::istringstream FNTStream (std::string((char*)RawFNT, (int)FNTSize));

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

		GLfloat u = (GLfloat)TextureX / (GLfloat)TextureWidth;
		GLfloat v = (GLfloat)TextureY / (GLfloat)TextureHeight;
		GLfloat w = (GLfloat)Width  / (GLfloat)TextureWidth;
		GLfloat h = (GLfloat)Height / (GLfloat)TextureHeight;

		glNewList(f->ListBase+i, GL_COMPILE);
			glBegin(GL_QUADS);
				glTexCoord2f(u,   v);	glVertex2i(OffsetX,       OffsetY);
				glTexCoord2f(u+w, v);	glVertex2i(OffsetX+Width, OffsetY);
				glTexCoord2f(u+w, v-h);	glVertex2i(OffsetX+Width, OffsetY-Height);
				glTexCoord2f(u,   v-h);	glVertex2i(OffsetX,       OffsetY-Height);
			glEnd();
			glTranslatef((GLfloat)Advance, 0, 0);
		glEndList();

		(*f->glyphs)[(wchar_t)Codepoint] = i;
	}

	// Load glyph texture
	const Handle ht = tex_load((FilenameBase+".tga").c_str());
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
	BoundGlyphs = f->glyphs;

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
		glyphmap::iterator it = BoundGlyphs->find(buf[i]);

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
