/*
 * OpenGL texture font
 *
 * Copyright (c) 2002 Jan Wassenberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Contact info:
 *   Jan.Wassenberg@stud.uni-karlsruhe.de
 *   http://www.stud.uni-karlsruhe.de/~urkt/
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "font.h"
#include "res.h"
#include "tex.h"
#include "ogl.h"
#include "posix.h"
#include "misc.h"


typedef struct
{
	Handle tex;
	uint list_base;
}
FONT;


#if 0

#include <ft2build.h>
//#include FT_FREETYPE_H


static FT_Library lib;


static void cleanup(void)
{
	FT_Done_FreeType(lib);
}


int build_font(const char* in_ttf, const char* out_fnt, const char* out_raw, int height)
{
	if(!lib)
	{
		FT_Init_FreeType(&lib);
		atexit(cleanup);
	}

	FT_Face face;
	if(FT_New_Face(lib, in_ttf, 0, &face))
		return -1;

	FT_Set_Pixel_Sizes(face, 0, height);
	const int tex_dim = 256;
	const int w = 24, h = 24;

	FILE* f = fopen(out_fnt, "w");
	if(!f)
		return -1;
	fprintf(f, "%s\n%d %d\n", out_raw, w, h);	/* header */

	u8* tex = (u8*)calloc(tex_dim*tex_dim, 2); /* GL_LUMINANCE_ALPHA fmt */

	int x = 0, y = 0;

	for(int c = 32; c < 128; c++)	/* for each (printable) char */
	{
		FT_Load_Char(face, c, FT_LOAD_RENDER);
		const u8* bmp = face->glyph->bitmap.buffer;

		/* copy glyph's bitmap into texture */
		for(int j = 0; j < face->glyph->bitmap.rows; j++)
		{
			u8* pos = &tex[(y+h-8-face->glyph->bitmap_top+j)*tex_dim*2 + (x+face->glyph->bitmap_left)*2];
			for(int i = 0; i < face->glyph->bitmap.width; i++)
			{
				*pos++ = *bmp;					/* luminance */
				*pos++ = (*bmp)? 0xff : 0x00;	/* alpha */
				bmp++;
			}
		}

		x += w;
		if(x + w >= tex_dim)
			x = 0, y += h;

		fprintf(f, "%d ", face->glyph->advance.x / 64);
	}

	fclose(f);

	/* write texture */
	f = fopen(out_raw, "wb");
	fwrite(tex, 2, tex_dim*tex_dim, f);
	fclose(f);

	free(tex);

	return 0;
}


#endif


static void font_dtor(void* p)
{
	FONT* font = (FONT*)p;
	glDeleteLists(font->list_base, 96);
}


u32 font_load(const char* fn)
{
	void* p;
	size_t size;
	HDATA* hd;
	Handle h = res_load(fn, H_FONT, font_dtor, p, size, hd);
	if(!h)
		return 0;

	int pos;	// current position in the file
	const char* file = (const char*)p;

	// read header
	char tex_filename[PATH_MAX];
	int x_stride, y_stride;	// glyph spacing in texture
	if(sscanf(file, "%s\n%d %d\n%n", tex_filename, &x_stride, &y_stride, &pos) != 3)
	{
		printf("Problem loading \"%s\": header is invalid", fn);
		return 0;
	}

	// read glyph widths
	int adv[128];
	for(int i = 32; i < 128; i++)
	{
		file += pos;
		if(sscanf(file, "%d %n", &adv[i], &pos) != 1)
		{
			printf("Problem loading \"%s\": glyph width array is invalid", fn);
			return 0;
		}
	}

	// load glyph texture
	const Handle tex = tex_load(tex_filename);
	if(!tex)
		return 0;
	tex_upload(tex);

	const int tex_dim = 256;
	const float du = (float)x_stride / (float)tex_dim;
	float u = 0, v = 0;

	// create a display list for each glyph
	const uint list_base = glGenLists(128);
	for(int c = 32; c < 128; c++)
	{
		const float w = (float)adv[c], h = (float)y_stride;	// glyph quad width/height
		const float tw = w / tex_dim, th = h / tex_dim;	// texture space width/height

		glNewList(list_base+c, GL_COMPILE);
		glBegin(GL_QUADS);
			glTexCoord2f(u, v+th);		glVertex2f(0, 0);
			glTexCoord2f(u+tw, v+th);	glVertex2f(w, 0);
			glTexCoord2f(u+tw, v);		glVertex2f(w, h);
			glTexCoord2f(u, v);			glVertex2f(0, h);
		glEnd();
		glTranslatef(w, 0, 0);
		glEndList();

		u += du;
		if(u + du > 1.f)
			u = 0.f, v += th;
	}

	FONT* font = (FONT*)hd->user;
	font->tex = tex;
	font->list_base = list_base;

	return h;
}


int font_bind(const Handle h)
{
	HDATA* hd = h_data(h, H_FONT);
	if(!hd)
		return -1;
	FONT* font = (FONT*)hd->user;

	tex_bind(font->tex);
	glListBase(font->list_base);

	return 0;
}


void glprintf(const char* fmt, ...)
{
	va_list args;
	char buf[1024]; buf[1023] = 0;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf)-1, fmt, args);
	va_end(args);

	glCallLists(strlen(buf), GL_UNSIGNED_BYTE, buf);
}
