// $Id: filemanip.cpp,v 1.1 2004/06/17 19:32:04 philip Exp $

#include "stdafx.h"

#include "filemanip.h"

// Ensure there's no padding added into the struct,
// in a very non-portable way
#pragma pack(push)
#pragma pack(1)
// For gcc, use "} TGA_HEADER __attribute__ ((packed));" instead of this

typedef struct
{
	char  identsize;		// size of ID field that follows 18 byte header (0 usually)
	char  colourmaptype;	// type of colour map 0=none, 1=has palette
	char  imagetype;		// type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

	short colourmapstart;	// first colour map entry in palette
	short colourmaplength;	// number of colours in palette
	char  colourmapbits;	// number of bits per palette entry 15,16,24,32

	short xstart;			// image x origin
	short ystart;			// image y origin
	short width;			// image width in pixels
	short height;			// image height in pixels
	char  bits;				// image bits per pixel 8,16,24,32
	char  descriptor;		// image descriptor bits (vh flip bits)
} TGA_HEADER;

#pragma pack(pop)

// Convert the RGB image to 8-bit greyscale and output
void RGB_OutputGreyscaleTGA(unsigned char* image_data, int width, int height, int pitch, wxFFile& file)
{
	assert(sizeof(TGA_HEADER) == 18);

	TGA_HEADER header;

	header.identsize = 0;
	header.colourmaptype = 0;
	header.imagetype = 3;
	header.xstart = 0;
	header.ystart = 0;
	header.width = (short)width;
	header.height = (short)height;
	header.bits = 8;
	header.descriptor = 0;

	file.Write(&header, sizeof(header));

	// Start from the bottom, so things look the right way up
	image_data += height*pitch;

	for (int y = 0; y < height; ++y)
	{
		image_data -= pitch;

		for (int x = 0; x < width; ++x)
			file.Write(&image_data[x*3], 1);
	}
}

std::set<wchar_t> AnalyseChars(wxString filename)
{
	wxFFile File (filename, "rb");
	if (! File.IsOpened())
		throw "Cannot open file";

	enum { 
		UNKNOWN,
		UTF8,
		UTF16SE, // same-endian as this machine
		UTF16DE  // different-endian
	} Format = UNKNOWN;

	unsigned short BOM;
	File.Read(&BOM, 2);
	if (BOM == 0xFEFF)
		Format = UTF16SE;
	else if (BOM == 0xFFFE)
		Format = UTF16DE;
	else {
		// Make an educated guess based on the first byte
		// If it's like "_\0" it's probably little-endian,
		// if it's like "\0_" it's probably big-endian
		if ((BOM & 0xFF00) == 0)
			Format = UTF16SE;
		else if ((BOM & 0x00FF) == 0)
			Format = UTF16DE;
		else
			throw "Can't determine endianness of source file - make sure it's UTF16 and starts with a simple letter";
			// (Maybe it's UTF8, but I don't want to bother with that)
		
		File.Seek(0); // Make sure the first character is included in the list
	}

	std::set<wchar_t> Chars;
	if (Format == UTF16SE)
	{
		wchar_t c;
		while (! File.Eof())
		{
			File.Read(&c, 2);
			if (c != '\r' && c != '\n')
				Chars.insert(c);
		}
	}
	else if (Format == UTF16DE)
	{
		unsigned short c;
		while (! File.Eof())
		{
			if (File.Read(&c, 2) != 2) break; // abort if failed (i.e. eof)
			c = (c>>8)|((c&0xff)<<8); // swap bytes
			if (c != '\r' && c != '\n')
				Chars.insert(c);
		}
	}
	else
	{
		throw "Internal error - invalid file format";
	}

	return Chars;
}
