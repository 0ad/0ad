// $Id: packer.h,v 1.1 2004/06/17 19:32:04 philip Exp $

#include "font.h"

#include <set>

class PackedFont
{
public:
	PackedFont(FontRenderer* font, std::set<wchar_t> chars);
	~PackedFont();

	void Generate(bool ProgressCallback(float, wxString, void*), void* CallbackData);

	// Sizes are always a power of two
	int TextureWidth;
	int TextureHeight;

	// 24-bit (RGB) but greyscale image
	unsigned char* TextureData;

	// For the text file describing how to read all the glyphs
	wxString FontDefinition;

private:
	FontRenderer* Font;
	std::set<wchar_t> Chars;
};
