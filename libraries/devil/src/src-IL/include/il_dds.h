//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 02/21/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_dds.h
//
// Description: Reads from a DirectDraw Surface (.dds) file.
//
//-----------------------------------------------------------------------------


#ifndef DDS_H
#define DDS_H

#include "il_internal.h"


#ifdef _WIN32
	#pragma pack(push, dds_struct, 1)
#endif
typedef struct DDSHEAD
{
	ILbyte	Signature[4];

	ILuint	Size1;				// size of the structure (minus MagicNum)
	ILuint	Flags1; 			// determines what fields are valid
	ILuint	Height; 			// height of surface to be created
	ILuint	Width;				// width of input surface
	ILuint	LinearSize; 		// Formless late-allocated optimized surface size
	ILuint	Depth;				// Depth if a volume texture
	ILuint	MipMapCount;		// number of mip-map levels requested
	ILuint	AlphaBitDepth;		// depth of alpha buffer requested

	ILuint	NotUsed[10];

	ILuint	Size2;				// size of structure
	ILuint	Flags2;				// pixel format flags
	ILuint	FourCC;				// (FOURCC code)
	ILuint	RGBBitCount;		// how many bits per pixel
	ILuint	RBitMask;			// mask for red bit
	ILuint	GBitMask;			// mask for green bits
	ILuint	BBitMask;			// mask for blue bits
	ILuint	RGBAlphaBitMask;	// mask for alpha channel
	
    ILuint	ddsCaps1, ddsCaps2, ddsCaps3, ddsCaps4; // direct draw surface capabilities
	ILuint	TextureStage;
} IL_PACKSTRUCT DDSHEAD;
#ifdef _WIN32
	#pragma pack(pop, dds_struct)
#endif



// use cast to struct instead of RGBA_MAKE as struct is
//  much
typedef struct Color8888
{
	ILubyte r;		// change the order of names to change the 
	ILubyte g;		//  order of the output ARGB or BGRA, etc...
	ILubyte b;		//  Last one is MSB, 1st is LSB.
	ILubyte a;
} Color8888;


typedef struct Color888
{
	ILubyte r;		// change the order of names to change the 
	ILubyte g;		//  order of the output ARGB or BGRA, etc...
	ILubyte b;		//  Last one is MSB, 1st is LSB.
} Color888;


typedef struct Color565
{
	unsigned nBlue  : 5;		// order of names changes
	unsigned nGreen : 6;		//	byte order of output to 32 bit
	unsigned nRed	: 5;
} Color565;


typedef struct DXTColBlock
{
	ILshort col0;
	ILshort col1;

	// no bit fields - use bytes
	ILbyte row[4];
} DXTColBlock;

typedef struct DXTAlphaBlockExplicit
{
	ILshort row[4];
} DXTAlphaBlockExplicit;

typedef struct DXTAlphaBlock3BitLinear
{
	ILbyte alpha0;
	ILbyte alpha1;

	ILbyte stuff[6];
} DXTAlphaBlock3BitLinear;


// Defines

//Those 4 were added on 20040516 to make
//the written dds files more standard compliant
#define DDS_CAPS				0x00000001l
#define DDS_HEIGHT				0x00000002l
#define DDS_WIDTH				0x00000004l
#define DDS_PIXELFORMAT			0x00001000l

#define DDS_ALPHAPIXELS			0x00000001l
#define DDS_ALPHA				0x00000002l
#define DDS_FOURCC				0x00000004l
#define DDS_PITCH				0x00000008l
#define DDS_COMPLEX				0x00000008l
#define DDS_TEXTURE				0x00001000l
#define DDS_MIPMAPCOUNT			0x00020000l
#define DDS_LINEARSIZE			0x00080000l
#define DDS_VOLUME				0x00200000l
#define DDS_MIPMAP				0x00400000l
#define DDS_DEPTH				0x00800000l

#define DDS_CUBEMAP				0x00000200L
#define DDS_CUBEMAP_POSITIVEX	0x00000400L
#define DDS_CUBEMAP_NEGATIVEX	0x00000800L
#define DDS_CUBEMAP_POSITIVEY	0x00001000L
#define DDS_CUBEMAP_NEGATIVEY	0x00002000L
#define DDS_CUBEMAP_POSITIVEZ	0x00004000L
#define DDS_CUBEMAP_NEGATIVEZ	0x00008000L


#define IL_MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
            ((ILint)(ILbyte)(ch0) | ((ILint)(ILbyte)(ch1) << 8) |   \
            ((ILint)(ILbyte)(ch2) << 16) | ((ILint)(ILbyte)(ch3) << 24 ))

enum PixFormat
{
	PF_ARGB,
	PF_RGB,
	PF_DXT1,
	PF_DXT2,
	PF_DXT3,
	PF_DXT4,
	PF_DXT5,
	PF_UNKNOWN
};

// Global variables
extern DDSHEAD	Head;			// Image header
extern ILubyte	*CompData;		// Compressed data
extern ILuint	CompSize;		// Compressed size
extern ILuint	CompLineSize;		// Compressed line size
extern ILuint	CompFormat;		// Compressed format
extern ILimage	*Image;
extern ILint	Width, Height, Depth;
extern ILuint	BlockSize;

#define CUBEMAP_SIDES 6

// Internal functions
ILboolean	iLoadDdsInternal(ILvoid);
ILboolean	iIsValidDds(ILvoid);
ILboolean	iCheckDds(DDSHEAD *Head);
ILvoid		AdjustVolumeTexture(DDSHEAD *Head);
ILboolean	ReadData(ILvoid);
ILboolean	AllocImage(ILvoid);
ILboolean	Decompress(ILvoid);
ILboolean	ReadMipmaps(ILvoid);
ILvoid		DecodePixelFormat(ILvoid);
ILboolean	DecompressARGB(ILvoid);
ILboolean	DecompressDXT1(ILvoid);
ILboolean	DecompressDXT2(ILvoid);
ILboolean	DecompressDXT3(ILvoid);
ILboolean	DecompressDXT4(ILvoid);
ILboolean	DecompressDXT5(ILvoid);
ILvoid		CorrectPreMult(ILvoid);
ILvoid		GetBitsFromMask(ILuint Mask, ILuint *ShiftLeft, ILuint *ShiftRight);
ILboolean	iSaveDdsInternal(ILvoid);
ILboolean	WriteHeader(ILimage *Image, ILenum DXTCFormat);
ILushort	*CompressTo565(ILimage *Image);
ILuint		Compress(ILimage *Image, ILenum DXTCFormat);
ILboolean	GetBlock(ILubyte *Block, ILubyte *Data, ILimage *Image, ILuint XPos, ILuint YPos);
ILboolean	GetAlphaBlock(ILubyte *Block, ILubyte *Data, ILimage *Image, ILuint XPos, ILuint YPos);
ILvoid		ShortToColor565(ILushort Pixel, Color565 *Colour);
ILvoid		ShortToColor888(ILushort Pixel, Color888 *Colour);
ILushort	Color565ToShort(Color565 *Colour);
ILushort	Color888ToShort(Color888 *Colour);
ILuint		GenBitMask(ILushort ex0, ILushort ex1, ILuint NumCols, ILubyte *In, ILubyte *Alpha, Color888 *OutCol);
ILuint		ApplyBitMask(Color888 ex0, Color888 ex1, ILuint NumCols, ILubyte *In, ILubyte *Alpha, Color888 *OutCol, ILuint *DistLimit);
ILvoid		GenAlphaBitMask(ILubyte a0, ILubyte a1, ILuint Num, ILubyte *In, ILubyte *Mask, ILubyte *Out);
ILuint		RMSAlpha(ILubyte *Orig, ILubyte *Test);
ILuint		Distance(Color888 *c1, Color888 *c2);
ILuint		ChooseEndpoints(ILushort *ex0, ILushort *ex1, ILuint NumCols, ILubyte *Block, ILubyte *Alpha, ILuint ClosestDist);
ILvoid		ChooseAlphaEndpoints(ILubyte *Block, ILubyte *a0, ILubyte *a1);
ILvoid		CorrectEndDXT1(ILushort *ex0, ILushort *ex1, ILboolean HasAlpha);
ILvoid		PreMult(ILushort *Data, ILubyte *Alpha);



#endif//DDS_H
