// $Id: packer.cpp,v 1.1 2004/06/17 19:32:04 philip Exp $

#include "stdafx.h"

#include "packer.h"

#include <list>
#include <vector>

#include <math.h>

PackedFont::PackedFont(FontRenderer* font, std::set<wchar_t> chars)
{
	TextureData = NULL;
	TextureWidth = TextureHeight = 0;
	Font = font;
	Chars = chars;
}


PackedFont::~PackedFont()
{
	delete[] TextureData;
}


class GlyphInfo {
public:
	GlyphInfo() { c = 0; w = h = size = -1; }
	GlyphInfo(wchar_t _c, int _w, int _h) { c=_c; w=_w; h=_h; size=(w*w)+(h*h); }
	wchar_t c;
	int w, h;
	int x, y; // of top-left corner

	bool operator<(GlyphInfo& b) { return size > b.size; } // sort biggest first

private:
	int size; // used for sorting
};

typedef std::vector<GlyphInfo> GlyphsInfo;

struct GlyphCharSort : public std::binary_function<GlyphInfo&, GlyphInfo&, bool> {
	bool operator()(GlyphInfo& x, GlyphInfo& y) { return x.c < y.c; }
};

int next_power_of_two(int x)
{
	// See something like http://bob.allegronetwork.com/prog/tricks.html for an explanation
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x+1;
}

// To avoid a few ugly gotos. Returns true on collision.
inline bool TryFittingInternal(const int &x, const int &y, GlyphsInfo& glyphs, const GlyphsInfo::iterator& ThisGlyph)
{
	for (GlyphsInfo::iterator OtherGlyph = glyphs.begin(); OtherGlyph != ThisGlyph; ++OtherGlyph)
	{
		// Test for collisions - effectively trying to find a line that
		// can fit between the two rectangles
		if (x <= OtherGlyph->x+OtherGlyph->w
		 && x+ThisGlyph->w >= OtherGlyph->x
		 && y <= OtherGlyph->y+OtherGlyph->h
		 && y+ThisGlyph->h >= OtherGlyph->y) return true;
	}
	return false;
}


bool TryFitting(const int packing_precision, const int texture_width, const int texture_height, const bool randomness, GlyphsInfo& glyphs, bool ProgressCallback(float, wxString, void*), void* CallbackData)
{
	// A roughly O(n^3) algorithm, but with lots of attempted
	// optimisations that do occasionally work.

	int count = 0;

	int ProgressCurrent = 0;
	int ProgressMax = (int)glyphs.size();

	GlyphInfo* LastGlyph = NULL;
	for (GlyphsInfo::iterator ThisGlyph = glyphs.begin(); ThisGlyph != glyphs.end(); ++ThisGlyph, ++count)
	{
		if (ProgressCallback != NULL)
		{
			// Squaring makes the bar increase at a more constant rate
			float Progress = powf((float)ProgressCurrent++ / (float)ProgressMax, 2.0f);
			if (ProgressCallback(Progress, wxT("Fitting"), CallbackData))
				throw "Aborted";
		}

		// After the first few easy ones, try sticking things in randomly
		// and see if it turns out adequately.

		if (randomness && count > 32)
		{
			/*
			// except this is quite rubbish, so don't bother

			bool fitted = false;
			for (int i = 0; i < 16; ++i) // try several times
			{
				int x = 1 + rand() % (texture_width-ThisGlyph->w-1);
				int y = 1 + rand() % (texture_height-ThisGlyph->h-1);
				for (GlyphsInfo::iterator OtherGlyph = glyphs.begin(); OtherGlyph != ThisGlyph; ++OtherGlyph)
					if (box_collide(x, y, x+ThisGlyphRef.w, y+ThisGlyphRef.h, OtherGlyph->x, OtherGlyph->y, OtherGlyph->x+OtherGlyph->w, OtherGlyph->y+OtherGlyph->h))
						goto randomly_collided;

				ThisGlyph->x = x;
				ThisGlyph->y = y;
				goto fitted;

				randomly_collided: ;
			}
			*/
		}

		bool fitted = false;

		int starty, stepy;

		// Check whether this glyph is no smaller than the previous one
		if (LastGlyph && LastGlyph->w <= ThisGlyph->w && LastGlyph->h <= ThisGlyph->h)
		{
			// If so, carry on the fitting from where the last glyph left off,
			// because we've already discovered that there's no more
			// space above.
			--count;
			if (count & 1)
				stepy = -packing_precision;
			else
				stepy = packing_precision;
			starty = LastGlyph->y;
		}
		else
		{

			// Alternate between fitting glyphs at the top and bottom
			// to make things go a bit faster by minimising collisions
			if (count & 1)
				starty = texture_height-ThisGlyph->h-1, stepy = -packing_precision;
			else
				starty = 1, stepy = packing_precision;
		}

		for (int y = starty; y > 0 && y < texture_height-ThisGlyph->h; y += stepy)
		{
			// and alternate between left and right
			int startx, stepx;
			if (count & 2)
				startx = 1, stepx = packing_precision;
			else
				startx = texture_width-ThisGlyph->w-1, stepx = -packing_precision;

			int x;

/* If you have a multiple-processor machine and the Intel C++ compiler,
// this parallel version might be a little faster. (It's only a few
// tens of percents slower on a hyperthreaded computer)

// [Assuming it works at all, which it probably doesn't because I haven't
// tried compiling it recently]

			if (startx == 1)
			{
				int max = texture_width-ThisGlyph->w;
				#pragma omp parallel shared(startx, texture_width, ThisGlyph, stepx, glyphs)
				{
					#pragma omp for schedule(static) nowait 
					for (x = startx; x < max; x += stepx)
					{
						if (! TryFittingInternal(x, y, glyphs, ThisGlyph))
						{
							ThisGlyph->x = x;
							stepx = 1e9; // Ugly hack so the threads finish at the same time
						}
					}
				}
			}
			else
			{
				#pragma omp parallel shared(startx, texture_width, ThisGlyph, stepx, glyphs)
				{
					#pragma omp for schedule(static) nowait 
					for (x = startx; x > 0; x += stepx)
					{
						if (! TryFittingInternal(x, y, glyphs, ThisGlyph))
						{
							ThisGlyph->x = x;
							stepx = -1e9;
						}
					}
				}

			}

			if (stepx == 1e9 || stepx == -1e9)
			{
				ThisGlyph->y = y;
				goto fitted;
			}
*/
			if (startx == 1)
			{
				for (x = startx; x < texture_width-ThisGlyph->w; x += stepx)
				{
					if (! TryFittingInternal(x, y, glyphs, ThisGlyph))
					{
						ThisGlyph->x = x;
						fitted = true;
						break;
					}
				}
			}
			else
			{
				for (x = startx; x > 0; x += stepx)
				{
					if (! TryFittingInternal(x, y, glyphs, ThisGlyph))
					{
						ThisGlyph->x = x;
						fitted = true;
					}
				}
			}

			if (fitted)
			{						
				ThisGlyph->y = y;
				break;
			}


		}

		if (!fitted)
			// Couldn't fit glyph anywhere - give up
			return false;

		//fitted: ; // for lazy people who just want to goto

		LastGlyph = &*ThisGlyph;
	}
	return true;
}

bool TryFittingLots(int& texture_width, int& texture_height, GlyphsInfo& glyphs, bool ProgressCallback(float, wxString, void*), void* CallbackData)
{
	// Try the fairly fast method
	if (TryFitting(16, texture_width, texture_height, false, glyphs, ProgressCallback, CallbackData))
		return true;
		
	// Go a bit more carefully
	if (TryFitting(8,  texture_width, texture_height, false, glyphs, ProgressCallback, CallbackData))
		return true;

	// Maybe it just needs a little more precision?
	if (TryFitting(2,  texture_width, texture_height, false, glyphs, ProgressCallback, CallbackData))
		return true;

	// This takes a long time and doesn't usually achieve much, but try it anyway
	if (TryFitting(1,  texture_width, texture_height, false, glyphs, ProgressCallback, CallbackData))
		return true;

	// Try swapping the dimensions if it's currently rectangular
	if (texture_width != texture_height)
	{
		std::swap(texture_width, texture_height);
		if (TryFitting(1, texture_width, texture_height, false, glyphs, ProgressCallback, CallbackData))
			return true;
		// Didn't work, swap back again
		std::swap(texture_width, texture_height);
	}

	// Double the size and try again
	if (texture_height == texture_width)
		texture_width *= 2;
	else
		texture_height *= 2;

	if (TryFitting(32, texture_width, texture_height, true, glyphs, ProgressCallback, CallbackData))
		return true;

	// How much space can it need?!
	if (TryFitting(4,  texture_width, texture_height, false, glyphs, ProgressCallback, CallbackData))
		return true;

	// Give up
	return false;
}

void PackedFont::Generate(bool ProgressCallback(float, wxString, void*) = NULL, void* CallbackData = NULL)
{
	GlyphsInfo glyphs;

	int total_area = 0;

	std::vector<wchar_t> MissingGlyphs;

	int ProgressCurrent = 0;
	int ProgressMax = (int)Chars.size();

	std::list<GlyphInfo> SortableGlyphs;

	for (std::set<wchar_t>::iterator c = Chars.begin(); c != Chars.end(); ++c)
	{
		if (ProgressCallback != NULL)
			if (ProgressCallback((float)ProgressCurrent++ / (float)ProgressMax, wxT("Measuring"), CallbackData))
				throw "Aborted";

		int w, h;
		Font->LoadGlyph(*c);
		if (Font->Missing)
		{
			// Was the missing glyph missing?
			if (*c == 0xFFFD)
			{
				// Just use a question mark. (Surely *every* font has a question mark...)
				Font->LoadGlyph('?');
				if (Font->Missing)
					throw "Can't find either a missing glyph symbol or a question mark!";
				Font->GetBoundingBox(w, h);
				SortableGlyphs.push_back( GlyphInfo('?', w, h) );
				total_area += w*h;
			}
			else
			{
				MissingGlyphs.push_back(*c);
			}
		}
		else
		{
			Font->GetBoundingBox(w, h);
			SortableGlyphs.push_back( GlyphInfo(*c, w, h) );
			total_area += w*h;
		}
	}
	SortableGlyphs.sort();
	// Convert into a vector for a little more speed
	for (std::list<GlyphInfo>::iterator i = SortableGlyphs.begin(); i != SortableGlyphs.end(); ++i)
	{
		glyphs.push_back(*i);
	}

	// Calculate minimum size square to hold all glyphs
	int texture_width, texture_height;
	texture_width = texture_height = next_power_of_two(int( sqrt((double)total_area) ));

	// See if a smaller rectangle ought to still fit
	if (texture_width*texture_height > total_area*2)
		texture_height /= 2;
	
	if (! TryFittingLots(texture_width, texture_height, glyphs, ProgressCallback, CallbackData))
		throw "Failed to fit all the glyphs, too many times.";

	TextureData = new unsigned char[texture_width*texture_height*3];
	memset(TextureData, 0, texture_width*texture_height*3);
	TextureWidth = texture_width;
	TextureHeight = texture_height;

	FontDefinition = wxEmptyString;
	FontDefinition += wxT("100\n"); // version number
	FontDefinition << TextureWidth << wxT(" ") << TextureHeight << wxT("\n"); // size of texture
	FontDefinition << (int)glyphs.size() << wxT("\n"); // number of glyphs
	FontDefinition << Font->GetLineSpacing() << wxT("\n");

	SortableGlyphs.clear();
	for (GlyphsInfo::iterator i = glyphs.begin(); i != glyphs.end(); ++i)
	{
		SortableGlyphs.push_back(*i);
	}
	SortableGlyphs.sort(GlyphCharSort());

	ProgressCurrent = 0;
	ProgressMax = (int)glyphs.size();
	for (std::list<GlyphInfo>::iterator ThisGlyph = SortableGlyphs.begin(); ThisGlyph != SortableGlyphs.end(); ++ThisGlyph)
	{
		if (ProgressCallback != NULL)
			if (ProgressCallback((float)ProgressCurrent++ / (float)ProgressMax, wxT("Rendering"), CallbackData))
				throw "Aborted";

		Font->LoadGlyph(ThisGlyph->c);
		Font->RenderGlyph(TextureData, ThisGlyph->x, ThisGlyph->y, texture_width, texture_height, texture_width*3, true);
		int offset_x, offset_y, advance;
		Font->GetMetrics(offset_x, offset_y, advance);
		FontDefinition	<< (int)ThisGlyph->c
						<< wxT(" ") << ThisGlyph->x
						<< wxT(" ") << (TextureHeight - ThisGlyph->y) // because it's stored upside-down
						<< wxT(" ") << ThisGlyph->w
						<< wxT(" ") << ThisGlyph->h
						<< wxT(" ") << offset_x
						<< wxT(" ") << offset_y
						<< wxT(" ") << advance
						<< wxT("\n");
	}

	if (MissingGlyphs.size())
	{
		wxString Error = wxString::Format(wxT("WARNING: %d glyphs were missing: "), MissingGlyphs.size());
		for (size_t i = 0; i < MissingGlyphs.size(); ++i)
		{
			if (i) Error += wxT(", ");
			if (i == 20)
			{
				Error += wxT("...");
				break;
			}
			Error += wxString::Format(wxT("%d"), MissingGlyphs[i]);
		}
		wxLogWarning(Error);
	}
}
