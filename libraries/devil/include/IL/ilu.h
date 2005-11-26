//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 06/23/2002 <--Y2K Compliant! =]
//
// Filename: IL/ilu.h
//
// Description: The main include file for ILU
//
//-----------------------------------------------------------------------------


#ifndef __ilu_h_
#ifndef __ILU_H__

#define __ilu_h_
#define __ILU_H__

#include <IL/il.h>

#ifdef __cplusplus
extern "C" {
#endif
/*
#ifdef _WIN32
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		#ifndef IL_STATIC_LIB
			#pragma comment(lib, "DevIL_DLL.lib")
			#ifndef _IL_BUILD_LIBRARY
				#pragma comment(lib, "DevILU_DLL.lib")
			#endif
		#else
			#ifndef _IL_BUILD_LIBRARY
				#ifdef  IL_DEBUG
					#pragma comment(lib, "DevILU_DBG.lib")
				#else
					#pragma comment(lib, "DevILU.lib")
				#endif//IL_DEBUG
			#endif
		#endif
	#endif
#endif
*/


#define ILU_VERSION_1_6_7					1
#define ILU_VERSION							167


#define ILU_FILTER							0x2600
#define ILU_NEAREST							0x2601
#define ILU_LINEAR							0x2602
#define ILU_BILINEAR						0x2603
#define ILU_SCALE_BOX						0x2604
#define ILU_SCALE_TRIANGLE					0x2605
#define ILU_SCALE_BELL						0x2606
#define ILU_SCALE_BSPLINE					0x2607
#define ILU_SCALE_LANCZOS3					0x2608
#define ILU_SCALE_MITCHELL					0x2609


// Error types
#define ILU_INVALID_ENUM					0x0501
#define ILU_OUT_OF_MEMORY					0x0502
#define ILU_INTERNAL_ERROR					0x0504
#define ILU_INVALID_VALUE					0x0505
#define ILU_ILLEGAL_OPERATION				0x0506
#define ILU_INVALID_PARAM					0x0509


// Values
#define ILU_PLACEMENT						0x0700
#define ILU_LOWER_LEFT						0x0701
#define ILU_LOWER_RIGHT						0x0702
#define ILU_UPPER_LEFT						0x0703
#define ILU_UPPER_RIGHT						0x0704
#define ILU_CENTER							0x0705
#define ILU_CONVOLUTION_MATRIX				0x0710
#define ILU_VERSION_NUM						IL_VERSION_NUM
#define ILU_VENDOR							IL_VENDOR


// Filters
/*
#define ILU_FILTER_BLUR						0x0803
#define ILU_FILTER_GAUSSIAN_3x3				0x0804
#define ILU_FILTER_GAUSSIAN_5X5				0x0805
#define ILU_FILTER_EMBOSS1					0x0807
#define ILU_FILTER_EMBOSS2					0x0808
#define ILU_FILTER_LAPLACIAN1				0x080A
#define ILU_FILTER_LAPLACIAN2				0x080B
#define ILU_FILTER_LAPLACIAN3				0x080C
#define ILU_FILTER_LAPLACIAN4				0x080D
#define ILU_FILTER_SHARPEN1					0x080E
#define ILU_FILTER_SHARPEN2					0x080F
#define ILU_FILTER_SHARPEN3					0x0810
*/


typedef struct ILinfo
{
	ILuint	Id;					// the image's id
	ILubyte	*Data;				// the image's data
	ILuint	Width;				// the image's width
	ILuint	Height;				// the image's height
	ILuint	Depth;				// the image's depth
	ILubyte	Bpp;				// bytes per pixel (not bits) of the image
	ILuint	SizeOfData;			// the total size of the data (in bytes)
	ILenum	Format;				// image format (in IL enum style)
	ILenum	Type;				// image type (in IL enum style)
	ILenum	Origin;				// origin of the image
	ILubyte	*Palette;			// the image's palette
	ILenum	PalType;			// palette type
	ILuint	PalSize;			// palette size
	ILenum	CubeFlags;			// flags for what cube map sides are present
	ILuint	NumNext;			// number of images following
	ILuint	NumMips;			// number of mipmaps
	ILuint	NumLayers;			// number of layers
} ILinfo;


typedef struct ILpointf
{
	ILfloat x, y;
} ILpointf;

typedef struct ILpointi
{
	ILint x, y;
} ILpointi;


// ImageLib Utility Functions
ILAPI ILboolean			ILAPIENTRY iluAlienify(ILvoid);
ILAPI ILboolean			ILAPIENTRY iluBlurAvg(ILuint Iter);
ILAPI ILboolean			ILAPIENTRY iluBlurGaussian(ILuint Iter);
ILAPI ILboolean			ILAPIENTRY iluBuildMipmaps(ILvoid);
ILAPI ILuint			ILAPIENTRY iluColoursUsed(ILvoid);
ILAPI ILboolean			ILAPIENTRY iluCompareImage(ILuint Comp);
ILAPI ILboolean			ILAPIENTRY iluContrast(ILfloat Contrast);
ILAPI ILboolean			ILAPIENTRY iluCrop(ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth);
ILAPI ILvoid			ILAPIENTRY iluDeleteImage(ILuint Id);
ILAPI ILboolean			ILAPIENTRY iluEdgeDetectE(ILvoid);
ILAPI ILboolean			ILAPIENTRY iluEdgeDetectP(ILvoid);
ILAPI ILboolean			ILAPIENTRY iluEdgeDetectS(ILvoid);
ILAPI ILboolean			ILAPIENTRY iluEmboss(ILvoid);
ILAPI ILboolean			ILAPIENTRY iluEnlargeCanvas(ILuint Width, ILuint Height, ILuint Depth);
ILAPI ILboolean			ILAPIENTRY iluEnlargeImage(ILfloat XDim, ILfloat YDim, ILfloat ZDim);
ILAPI ILboolean			ILAPIENTRY iluEqualize(ILvoid);
ILAPI const ILstring	ILAPIENTRY iluErrorString(ILenum Error);
ILAPI ILboolean			ILAPIENTRY iluFlipImage(ILvoid);
ILAPI ILboolean			ILAPIENTRY iluGammaCorrect(ILfloat Gamma);
ILAPI ILuint			ILAPIENTRY iluGenImage(ILvoid);
ILAPI ILvoid			ILAPIENTRY iluGetImageInfo(ILinfo *Info);
ILAPI ILint				ILAPIENTRY iluGetInteger(ILenum Mode);
ILAPI ILvoid			ILAPIENTRY iluGetIntegerv(ILenum Mode, ILint *Param);
ILAPI const ILstring	ILAPIENTRY iluGetString(ILenum StringName);
ILAPI ILvoid			ILAPIENTRY iluImageParameter(ILenum PName, ILenum Param);
ILAPI ILvoid			ILAPIENTRY iluInit(ILvoid);
ILAPI ILboolean			ILAPIENTRY iluInvertAlpha(ILvoid);
ILAPI ILuint			ILAPIENTRY iluLoadImage(const ILstring FileName);
ILAPI ILboolean			ILAPIENTRY iluMirror(ILvoid);
ILAPI ILboolean			ILAPIENTRY iluNegative(ILvoid);
ILAPI ILboolean			ILAPIENTRY iluNoisify(ILclampf Tolerance);
ILAPI ILboolean			ILAPIENTRY iluPixelize(ILuint PixSize);
ILAPI ILvoid			ILAPIENTRY iluRegionfv(ILpointf *Points, ILuint n);
ILAPI ILvoid			ILAPIENTRY iluRegioniv(ILpointi *Points, ILuint n);
ILAPI ILboolean			ILAPIENTRY iluReplaceColour(ILubyte Red, ILubyte Green, ILubyte Blue, ILfloat Tolerance);
ILAPI ILboolean			ILAPIENTRY iluRotate(ILfloat Angle);
ILAPI ILboolean			ILAPIENTRY iluRotate3D(ILfloat x, ILfloat y, ILfloat z, ILfloat Angle);
ILAPI ILboolean			ILAPIENTRY iluSaturate1f(ILfloat Saturation);
ILAPI ILboolean			ILAPIENTRY iluSaturate4f(ILfloat r, ILfloat g, ILfloat b, ILfloat Saturation);
ILAPI ILboolean			ILAPIENTRY iluScale(ILuint Width, ILuint Height, ILuint Depth);
ILAPI ILboolean			ILAPIENTRY iluScaleColours(ILfloat r, ILfloat g, ILfloat b);
ILAPI ILboolean			ILAPIENTRY iluSharpen(ILfloat Factor, ILuint Iter);
ILAPI ILboolean			ILAPIENTRY iluSwapColours(ILvoid);
ILAPI ILboolean			ILAPIENTRY iluWave(ILfloat Angle);

#define iluColorsUsed	iluColoursUsed
#define iluSwapColors	iluSwapColours
#define iluReplaceColor	iluReplaceColour
#define iluScaleColor	iluScaleColour

#ifdef __cplusplus
}
#endif

#endif // __ILU_H__
#endif // __ilu_h_
