//***********************************************************
//
// Name:		Bitmap.H
// Author:		Poya Manouchehri
//
// Description: CBitmap operates on a bitmap. Currently it
//				can load BMPs and TGAs. All bitmaps (even 8-bit
//				ones) are converted into full RGBAs.
//
//***********************************************************

#ifndef BITMAP_H
#define BITMAP_H

#include <stdio.h>
#include "Types.H"
#include "FileResource.H"


//image types
#define FILE_TYPE_NOSUPPORT	(0)
#define FILE_TYPE_BMP		(1)
#define FILE_TYPE_TGA		(2)

//this structure holds header data for a TGA file
typedef struct
{
	unsigned char	IDLength;
	unsigned char	MapType;
	unsigned char	ImageType;
	unsigned short	MapOrigin;
	unsigned short	MapLength;
	unsigned char	MapEntrySize;
	unsigned short	XOrigin;
	unsigned short	YOrigin;
	unsigned short	Width;
	unsigned short	Height;
	unsigned char	BPP;
	unsigned char	Descriptor;
} TGAHeader;

class CBitmap : public CFileResource
{
	public:
		CBitmap ();
		virtual ~CBitmap ();

		//Load the image from a file
		virtual FRESULT LoadBitmap (char *path, RESOURCETYPE type);

		//Creates the bits for the bitmap
		virtual FRESULT CreateBitmap (RESOURCETYPE type, char *name, int width, int height);

		//Get the pointer to the image data
		unsigned char *GetBits () { return m_pBits; }

		//Get width and height
		int	GetWidth () { return m_Width; }
		int GetHeight () { return m_Height; }

	private:
		//Get the file format
		int GetImageType (FILE *file);
		//Load a bmp file
		FRESULT LoadBMP (FILE *file);
		//Load a TGA file
		FRESULT LoadTGA (FILE *file);
		//Load the actual bits from a file. the table is NULL
		//unless the image is 8-bit
		FRESULT LoadData (FILE *file, int bpp, RGBQUAD *table);

	protected:
		//release the memory of the bits
		void DestroyData ();

	protected:
		unsigned char	*m_pBits;		//Bitmap's bits
		int				m_Width;
		int				m_Height;
};


#endif