#ifndef _NPFONT_H
#define _NPFONT_H

// necessary includes
#include "CStr.h"
#include "Texture.h"

/////////////////////////////////////////////////////////////////////////////////////////
// NPFont:
class NPFont 
{
public:
	struct CharData {
		int _width;						// total width in pixels
		int _widthA,_widthB,_widthC;	// ABC widths
	};

public:
	// create - create font from given font file
	static NPFont* create(const char* fontfilename);
	// destructor
	~NPFont();

	// accessor for name (font file name)
	const char* name() const { return (const char*) _name; }
	
	// accessors for font metrics 
	int width(int c) const { assert(c>=0 && c<128); return _chars[c]._width; }
	int height() const { return _metrics._height; }
	int descent() const { return _metrics._descent; }
	int maxcharwidth() const { return _metrics._maxcharwidth; }

	// accessors for texture data
	int textureWidth() const { return _texwidth; }
	int textureHeight() const { return _texheight; }
	CTexture& texture() { return _texture; }



	int numCols() const { return _numCols; }
	int numRows() const { return _numRows; }

	// accessor for character data
	const CharData& chardata(char c) const { 
		assert(c>=0 && c<128);
		return _chars[c]; 
	}

	void GetOutputStringSize(const char* str,int& sx,int& sy);

private:
	// constructor (private - all fonts created through create())
	NPFont();

	// font name
	CStr _name;
	// font texture width
	int _texwidth;
	// font texture height
	int _texheight;
	// font texture 
	CTexture _texture;
	// font metrics
	struct {
		int _height;
		int _descent;
		int _maxcharwidth;
	} _metrics;
	// number of rows of characters
	int _numRows;
	// number of columns of characters
	int _numCols;
	// details about specific characters in this font
	CharData _chars[128];
};
/////////////////////////////////////////////////////////////////////////////////////////


#endif

