// $Id: font.h,v 1.2 2004/07/16 15:32:34 philip Exp $

#ifndef _FONT_H_
#define _FONT_H_

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_BBOX_H


// XXX: Kerning ( = hard? got to store giant table for OpenGL to use)

// XXX: Right-to-left text



// Make my IDE a little happier:
#if 0
 #include "freetype/freetype.h"
 #include "freetype/freetype_defs.h"
 #include "freetype/ftimage.h"
 #include "freetype/fttypes.h"
 #include "freetype/ftbbox.h"
 #include "freetype/ftoutln.h"
#endif

class FontRenderer
{
public:

	// Two fonts are required - a primary (0) font which will be used first,
	// and a secondary (1) font for filling in missing glyphs.
	// (The secondary font should usually be Arial Unicode MS).
	FontRenderer(const char* filename0, const char* filename1, int ptsize, bool unpatented_hinting);

	~FontRenderer();
	
	// Generate the glyph for the given Unicode code point
	void LoadGlyph(const int charcode);

	// Put the appropriate pixel sizes into width and height
	void GetBoundingBox(int& width, int& height);

	// Copy the glyph onto a 24-bit RGB image buffer
	//   pos_left / pos_top: Position to put the origin (usually bottom left corner) of the glyph.
	//                       Altered by the function to point towards the next character.
	//   width / height: of bitmap, to do clipping
	//   pitch: Bytes per row, including any padding
	//   single_glyph: If true, doesn't update the cursor and draws with the bounding box at pos_left/top
	void RenderGlyph(unsigned char* rgb_buffer, int& pos_left, int& pos_top, int width, int height, int pitch, bool single_glyph);

	// Returns baseline-to-baseline distance, including the desired leading
	int GetLineSpacing();

	// Supplies some information for positioning glyphs
	void GetMetrics(int& offset_x, int& offset_y, int& advance);

	// Set by LoadGlyph if it can't find the right glyph
	bool Missing;

	/////////////////////////////////////////////////////////
	
	// Font settings, set by users and used by this class:

	// Shear angle (in degrees) for fake italics
	int Italicness;

	// 0 for standard; otherwise draws n+1 overlapping copies of the text
	int Boldness;

	// Additional spacing between each non-zero-width glyph and the next
	int Tracking;

	// Additional spacing between lines
	int Leading;

	// Whether to draw an outlined font
	bool Outline;

private:
	FT_Library FontLibrary0;
	FT_Library FontLibrary1;
	FT_Face FontFace0;
	FT_Face FontFace1;

	FT_Face LastFontFace; // = FontFace(0|1), depending on the last LoadGlyph
};

#endif // _FONT_H_
