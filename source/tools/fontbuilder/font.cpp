// $Id: font.cpp,v 1.1 2004/06/17 19:32:04 philip Exp $

#include "stdafx.h"

#include "font.h"
#include <math.h>

FontRenderer::FontRenderer(const char* filename0, const char* filename1, int ptsize)
{
	int error;
	
	error = FT_Init_FreeType(&FontLibrary0);
	if (error)
	{
		throw "Error initialising FreeType";
	}

	error = FT_Init_FreeType(&FontLibrary1);
	if (error)
	{
		FT_Done_FreeType(FontLibrary0);
		throw "Error initialising FreeType";
	}

	error = FT_New_Face(
		FontLibrary0,
		filename0,
		0, // index of face inside font file
		&FontFace0
	);
	if (error)
	{
		FT_Done_FreeType(FontLibrary0);
		FT_Done_FreeType(FontLibrary1);
		throw "Error loading primary font";
	}

	error = FT_New_Face(
		FontLibrary1,
		filename1,
		0, // index of face inside font file
		&FontFace1
	);
	if (error)
	{
		FT_Done_Face(FontFace0);
		FT_Done_FreeType(FontLibrary0);
		FT_Done_FreeType(FontLibrary1);
		throw "Error loading secondary font face";
	}

	error = FT_Set_Char_Size(
		FontFace0,
		0,			// char_width in 1/64th of points
		ptsize*64,	// char_height in 1/64th of points
		72,	// horizontal device resolution
		72	// vertical device resolution
	);
	if (error)
	{
		FT_Done_Face(FontFace0);
		FT_Done_Face(FontFace1);
		FT_Done_FreeType(FontLibrary0);
		FT_Done_FreeType(FontLibrary1);
		throw "Error loading scalable character from primary font - is this a TrueType font?";
	}

	error = FT_Set_Char_Size(
		FontFace1,
		0,			// char_width in 1/64th of points
		ptsize*64,	// char_height in 1/64th of points
		72,	// horizontal device resolution
		72	// vertical device resolution
	);
	if (error)
	{
		FT_Done_Face(FontFace0);
		FT_Done_Face(FontFace1);
		FT_Done_FreeType(FontLibrary0);
		FT_Done_FreeType(FontLibrary1);
		throw "Error loading scalable character from secondary font - is this a TrueType font?";
	}

	Boldness = 0;
	Italicness = 0;
	Tracking = 0;
	Leading = 0;
}

FontRenderer::~FontRenderer()
{
	FT_Done_Face(FontFace0);
	FT_Done_Face(FontFace1);
	FT_Done_FreeType(FontLibrary0);
	FT_Done_FreeType(FontLibrary1);
}

#define deg2rad(a) ((double)a * 3.14159265358979323846 / 180.0)

void FontRenderer::LoadGlyph(const int charcode)
{
	FT_Face FontFace = FontFace0;
	int glyph_index = FT_Get_Char_Index(FontFace0, charcode);

	Missing = false;
	if (glyph_index == 0)
	{
		// Can't find glyph in primary font - switch to secondary
		FontFace = FontFace1;
		glyph_index = FT_Get_Char_Index(FontFace1, charcode);

		// Still can't find it - use the missing glyph symbol
		if (glyph_index == 0)
		{
			glyph_index = FT_Get_Char_Index(FontFace1, 0xFFFD);
			Missing = true;
		}
	}


	int error;

	if (Italicness)
	{
		// Start with identity matrix  (rotations could be added in here easily)
		FT_Matrix mat;
		mat.xx = mat.yy = (FT_Fixed)(0x10000L);
		mat.xy = mat.yx = (FT_Fixed)(0x00000L);
		// Apply shear
		FT_Fixed f = (FT_Fixed)(tan(deg2rad(Italicness)) * 0x10000L);
		mat.xy += FT_MulFix(f, mat.xx);
		mat.yy += FT_MulFix(f, mat.yx);

		FT_Set_Transform(FontFace, &mat, 0);
	}

	error = FT_Load_Glyph(FontFace, glyph_index, FT_LOAD_DEFAULT);
	if (error)
		throw "Error loading glyph";


	if (Outline)
	{
		wxLogFatalError(wxT("Can't do this yet"));
		/*
		// Outline renderer which doesn't actually work:

		assert(FontFace->glyph->format == FT_GLYPH_FORMAT_OUTLINE);

		//FT_Outline *FontOutline = new FT_Outline;
		//error = FT_Outline_New(FontLibrary, 1024, 1024, FontOutline);
		//assert(!error);
		FT_Outline *FontOutline = &FontFace->glyph->outline;

		FT_BBox BBox;

		error = FT_Outline_Get_BBox(FontOutline, &BBox);
		assert(!error);

		// Translate back to (0,0)
		FT_Outline_Translate(FontOutline, -BBox.xMin, -BBox.yMin);

		FT_Bitmap *Bmp = new FT_Bitmap;
		Bmp->pixel_mode = FT_PIXEL_MODE_GRAY;
		Bmp->num_grays = 256;
		FontFace->glyph->bitmap_left = (BBox.xMin) >> 6;
		FontFace->glyph->bitmap_top = (BBox.yMax + 31) >> 6;
		Bmp->width = (BBox.xMax - BBox.xMin) >> 6;
		Bmp->pitch = 3*Bmp->width;
		Bmp->rows = (BBox.yMax - BBox.yMin) >> 6;
		Bmp->buffer = new unsigned char[Bmp->pitch * Bmp->rows];
		memset(Bmp->buffer, 0, sizeof(unsigned char) * Bmp->pitch * Bmp->rows);

		error = FT_Outline_Get_Bitmap(FontLibrary, FontOutline, Bmp);
		assert(!error);

		FontFace->glyph->bitmap = *Bmp;

		error = FT_Outline_Done(FontLibrary, FontOutline);
		assert(!error);
		*/
	}
	else
	{
		// Simple version:
		error = FT_Render_Glyph(FontFace->glyph, FT_RENDER_MODE_NORMAL);
		if (error)
			throw "Error rendering glyph";
	}

	LastFontFace = FontFace;
}


void FontRenderer::GetBoundingBox(int& width, int& height)
{
	FT_Bitmap* bmp = &LastFontFace->glyph->bitmap;
	width = bmp->width + Boldness;
	height = bmp->rows;
}


void FontRenderer::RenderGlyph(unsigned char* rgb_buffer, int& pos_left, int& pos_top, int width, int height, int pitch, bool single_glyph)
{
	FT_Face FontFace = LastFontFace;

	// This function only tries to work on 8-bit greyscale glyphs
	if (FontFace->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
		throw "Error - invalid bitmap format";

	// Add desired spacing, but not after zero-width combining glyphs
	if (!single_glyph && FontFace->glyph->advance.x)
		pos_left += Tracking;

	FT_Bitmap* bmp = &FontFace->glyph->bitmap;

	// Pointers to the appropriate row of pixel data
	unsigned char* ImageRow = rgb_buffer;
	unsigned char* GlyphRow = bmp->buffer;

	int Left = pos_left;
	int Top = pos_top;
	if (!single_glyph)
	{
		Left += FontFace->glyph->bitmap_left;
		Top -= FontFace->glyph->bitmap_top;
	}

	// Point to the first line that needs to be drawn
	if (Top >= 0)
		ImageRow += Top*pitch;

	for (int GlyphY = 0; GlyphY < bmp->rows; ++GlyphY)
	{
		// Clip vertically
		if (Top+GlyphY < 0 || Top+GlyphY >= height)
		{
			GlyphRow += bmp->pitch;
			continue;
		}

		for (int GlyphX = -Boldness; GlyphX < bmp->width+Boldness; ++GlyphX)
		{
			// Clip horizontally
			if (Left+GlyphX < 0 || Left+GlyphX >= width)
				continue;

			int value = 0;

			if (! Boldness)
			{
				value = GlyphRow[GlyphX];
			}
			else
			{
				// Pretend to be bold - add lots of adjacent images
				// Calculate value =~ sum(in[x-n] .. in[x+n])
				
				// Work out valid limits for summing
				// (with weird shifts so that it does precisely
				// Boldness pixels)
				int min = GlyphX - ( Boldness >> 1);
				if (min < 0) min = 0;
				int max = GlyphX + ( (Boldness+1) >> 1) + 1;
				if (max > bmp->width) max = bmp->width;

				for (int x = min; x < max; ++x)
					value += GlyphRow[x];

			}

			// Additive composition of pixel onto image buffer
			value += ImageRow[(Left+GlyphX)*3];
			ImageRow[(Left+GlyphX)*3  ] =
			ImageRow[(Left+GlyphX)*3+1] =
			ImageRow[(Left+GlyphX)*3+2] = (unsigned char)(value > 255 ? 255 : value);

		}

		// Point everything at the next line
		ImageRow += pitch;
		GlyphRow += bmp->pitch;
	}

	if (!single_glyph)
	{
		// Advance the drawing-cursor ( >>6 due to 26.6-bit number format)
		pos_left += FontFace->glyph->advance.x >> 6;
		pos_top += FontFace->glyph->advance.y >> 6;
	}
}

int FontRenderer::GetLineSpacing()
{
	return Leading + (FontFace0->size->metrics.height >> 6);
}

void FontRenderer::GetMetrics(int& offset_x, int& offset_y, int& advance)
{
	offset_x = LastFontFace->glyph->bitmap_left;
	offset_y = LastFontFace->glyph->bitmap_top;
	advance = LastFontFace->glyph->advance.x >> 6;
	if (advance)
		offset_x += Tracking;
}
