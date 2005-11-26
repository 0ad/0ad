//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 02/19/2002 <--Y2K Compliant! =]
//
// Filename: IL/devil_internal_exports.h
//
// Description: Internal stuff for DevIL (IL, ILU and ILUT)
//
//-----------------------------------------------------------------------------

#ifndef IL_EXPORTS_H
#define IL_EXPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

#define IL_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define IL_MIN(a,b) (((a) < (b)) ? (a) : (b))


// Basic Palette struct
typedef struct ILpal
{
	ILubyte	*Palette;		// the image palette (if any)
	ILuint	PalSize;		// size of the palette (in bytes)
	ILenum	PalType;		// the palette types below (0x0500 range)
} ILpal;


// The Fundamental Image struct
typedef struct ILimage
{
	ILuint	Width;				// the image's width
	ILuint	Height;				// the image's height
	ILuint	Depth;				// the image's depth
	ILubyte	Bpp;				// bytes per pixel (now number of channels)
	ILubyte	Bpc;				// bytes per channel
	ILuint	Bps;				// bytes per scanline (components for IL)
	ILubyte	*Data;				// the image data
	ILuint	SizeOfData;			// the total size of the data (in bytes)
	ILuint	SizeOfPlane;		// SizeOfData in a 2d image, size of each plane slice in a 3d image (in bytes)
	ILenum	Format;				// image format (in IL enum style)
	ILenum	Type;				// image type (in IL enum style)
	ILenum	Origin;				// origin of the image
	ILpal	Pal;				// palette details
	ILuint	Duration;			// length of the time to display this "frame"
	ILenum	CubeFlags;			// cube map flags for sides present in chain
	struct	ILimage *Mipmaps;	// mipmapped versions of this image terminated by a NULL - usu. NULL
	struct	ILimage *Next;		// next image in the chain - usu. NULL
	struct	ILimage *Layers;	// subsequent layers in the chain - usu. NULL
	ILuint	NumNext;			// number of images following this one (0 when not parent)
	ILuint	NumMips;			// number of mipmaps (0 when not parent)
	ILuint	NumLayers;			// number of layers (0 when not parent)
	ILuint	*AnimList;			// animation list
	ILuint	AnimSize;			// animation list size
	ILvoid	*Profile;			// colour profile
	ILuint	ProfileSize;		// colour profile size
	ILuint	OffX, OffY;			// offset of the image
	ILubyte	*DxtcData;			// compressed data
	ILenum	DxtcFormat;			// compressed data format
	ILuint	DxtcSize;			// compressed data size
} ILimage;


// Memory functions
ILAPI ILvoid*	ILAPIENTRY ialloc(ILuint Size);
ILAPI ILvoid	ILAPIENTRY ifree(ILvoid *Ptr);
ILAPI ILvoid*	ILAPIENTRY icalloc(ILuint Size, ILuint Num);



// Internal library functions in IL
ILAPI ILimage*		ILAPIENTRY ilGetCurImage(ILvoid);
ILAPI ILvoid		ILAPIENTRY ilSetCurImage(ILimage *Image);
ILAPI ILvoid		ILAPIENTRY ilSetError(ILenum Error);
ILAPI ILvoid		ILAPIENTRY ilSetPal(ILpal *Pal);

//
// Utility functions
//
ILAPI ILubyte	ILAPIENTRY ilGetBppFormat(ILenum Format);
ILAPI ILubyte	ILAPIENTRY ilGetBppPal(ILenum PalType);
ILAPI ILubyte	ILAPIENTRY ilGetBppType(ILenum Type);
ILAPI ILenum	ILAPIENTRY ilGetTypeBpc(ILubyte Bpc);
ILAPI ILenum	ILAPIENTRY ilGetPalBaseType(ILenum PalType);
ILAPI ILuint	ILAPIENTRY ilNextPower2(ILuint Num);
ILAPI ILenum	ILAPIENTRY ilTypeFromExt(const ILstring FileName);
ILAPI ILvoid ILAPIENTRY ilReplaceCurImage(ILimage *Image);

//
// Image functions
//
ILAPI ILvoid	ILAPIENTRY	iBindImageTemp(ILvoid);
ILAPI ILboolean ILAPIENTRY	ilClearImage_(ILimage *Image);
ILAPI ILvoid	ILAPIENTRY	ilCloseImage(ILimage *Image);
ILAPI ILvoid	ILAPIENTRY	ilClosePal(ILpal *Palette);
ILAPI ILpal*	ILAPIENTRY	iCopyPal(ILvoid);
ILAPI ILboolean	ILAPIENTRY	ilCopyImageAttr(ILimage *Dest, ILimage *Src);
ILAPI ILimage*	ILAPIENTRY	ilCopyImage_(ILimage *Src);
ILAPI ILvoid	ILAPIENTRY	ilGetClear(ILvoid *Colours, ILenum Format, ILenum Type);
ILAPI ILuint	ILAPIENTRY	ilGetCurName(ILvoid);
ILAPI ILboolean	ILAPIENTRY	ilIsValidPal(ILpal *Palette);
ILAPI ILimage*	ILAPIENTRY	ilNewImage(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc);
ILAPI ILboolean ILAPIENTRY	ilResizeImage(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc);
ILAPI ILboolean ILAPIENTRY	ilTexImage_(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, ILvoid *Data);
ILAPI ILboolean ILAPIENTRY	ilTexSubImage_(ILimage *Image, ILvoid *Data);
ILAPI ILvoid*	ILAPIENTRY	ilConvertBuffer(ILuint SizeOfData, ILenum SrcFormat, ILenum DestFormat, ILenum SrcType, ILenum DestType, ILvoid *Buffer);
ILAPI ILimage*	ILAPIENTRY	iConvertImage(ILimage *Image, ILenum DestFormat, ILenum DestType);
ILAPI ILpal*	ILAPIENTRY	iConvertPal(ILpal *Pal, ILenum DestFormat);
ILAPI ILubyte*	ILAPIENTRY	iGetFlipped(ILimage *Image);


// Internal library functions in ILU
ILAPI ILimage* ILAPIENTRY iluRotate_(ILimage *Image, ILfloat Angle);
ILAPI ILimage* ILAPIENTRY iluRotate3D_(ILimage *Image, ILfloat x, ILfloat y, ILfloat z, ILfloat Angle);
ILAPI ILimage* ILAPIENTRY iluScale_(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth);

#ifdef __cplusplus
}
#endif

#endif//IL_EXPORTS_H
