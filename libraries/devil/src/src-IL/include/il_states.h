//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/24/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_states.h
//
// Description: State machine
//
//-----------------------------------------------------------------------------


#ifndef STATES_H
#define STATES_H

#include "il_internal.h"


ILboolean ilAble(ILenum Mode, ILboolean Flag);


#define IL_ATTRIB_STACK_MAX 32

ILuint ilCurrentPos = 0;  // Which position on the stack

//
// Various states
//

typedef struct IL_STATES
{
	// Origin states
	ILboolean	ilOriginSet;
	ILenum		ilOriginMode;
	// Format and type states
	ILboolean	ilFormatSet;
	ILboolean	ilTypeSet;
	ILenum		ilFormatMode;
	ILenum		ilTypeMode;
	// File mode states
	ILboolean	ilOverWriteFiles;
	// Palette states
	ILboolean	ilAutoConvPal;
	// Load fail states
	ILboolean	ilDefaultOnFail;
	// Key colour states
	ILboolean	ilUseKeyColour;
	// Compression states
	ILenum		ilCompression;
	// Interlace states
	ILenum		ilInterlace;
	// Quantization states
	ILenum		ilQuantMode;
	ILuint		ilNeuSample;
	ILuint		ilQuantMaxIndexs;
	// DXTC states
	ILboolean	ilKeepDxtcData;


	//
	// Format-specific states
	//

	ILboolean	ilTgaCreateStamp;
	ILuint		ilJpgQuality;
	ILboolean	ilPngInterlace;
	ILboolean	ilTgaRle;
	ILboolean	ilBmpRle;
	ILboolean	ilSgiRle;
	ILenum		ilJpgFormat;
	ILenum		ilDxtcFormat;
	ILenum		ilPcdPicNum;

	ILint		ilPngAlphaIndex;	// this index should be treated as an alpha key (most formats use this rather than having alpha in the palette), -1 for none
									// currently only used when writing out .png files and should obviously be set to -1 most of the time

	//
	// Format-specific strings
	//

	char		*ilTgaId;
	char		*ilTgaAuthName;
	char		*ilTgaAuthComment;
	char		*ilPngAuthName;
	char		*ilPngTitle;
	char		*ilPngDescription;
	char		*ilTifDescription;
	char		*ilTifHostComputer;
	char		*ilTifDocumentName;
	char		*ilTifAuthName;
	char		*ilCHeader;




} IL_STATES;

IL_STATES ilStates[IL_ATTRIB_STACK_MAX];


typedef struct IL_HINTS
{
	// Memory vs. Speed trade-off
	ILenum		MemVsSpeedHint;
	// Compression hints
	ILenum		CompressHint;

} IL_HINTS;

IL_HINTS ilHints;


#ifndef IL_NO_BMP
	#define IL_BMP_EXT "bmp dib "
#else
	#define IL_BMP_EXT ""
#endif

#ifndef IL_NO_CHEAD
	#define IL_CHEAD_EXT "h "
#else
	#define IL_CHEAD_EXT ""
#endif

#ifndef IL_NO_CUT
	#define IL_CUT_EXT "cut "
#else
	#define IL_CUT_EXT ""
#endif

#ifndef IL_NO_DCX
	#define IL_DCX_EXT "dcx "
#else
	#define IL_DCX_EXT ""
#endif

#ifndef IL_NO_DDS
	#define IL_DDS_EXT "dds "
#else
	#define IL_DDS_EXT ""
#endif

#ifndef IL_NO_GIF
	#define IL_GIF_EXT "gif "
#else
	#define IL_GIF_EXT ""
#endif

#ifndef IL_NO_ICO
	#define IL_ICO_EXT "ico cur "
#else
	#define IL_ICO_EXT ""
#endif

#ifndef IL_NO_JPG
	#define IL_JPG_EXT "jpg jpe jpeg "
#else
	#define IL_JPG_EXT ""
#endif

#ifndef IL_NO_LIF
	#define IL_LIF_EXT "lif "
#else
	#define IL_LIF_EXT ""
#endif

#ifndef IL_NO_MDL
	#define IL_MDL_EXT "mdl "
#else
	#define IL_MDL_EXT ""
#endif

#ifndef IL_NO_MNG
	#define IL_MNG_EXT "mng jng "
#else
	#define IL_MNG_EXT ""
#endif

#ifndef IL_NO_PCX
	#define IL_PCX_EXT "pcx "
#else
	#define IL_PCX_EXT ""
#endif

#ifndef IL_NO_PIC
	#define IL_PIC_EXT "pic "
#else
	#define IL_PIC_EXT ""
#endif

#ifndef IL_NO_PIX
	#define IL_PIX_EXT "pix "
#else
	#define IL_PIX_EXT ""
#endif

#ifndef IL_NO_PNG
	#define IL_PNG_EXT "png "
#else
	#define IL_PNG_EXT ""
#endif

#ifndef IL_NO_PNM
	#define IL_PNM_EXT "pbm pgm pnm ppm "
#else
	#define IL_PNM_EXT ""
#endif

#ifndef IL_NO_PSD
	#define IL_PSD_EXT "psd pdd "
#else
	#define IL_PSD_EXT ""
#endif

#ifndef IL_NO_PSP
	#define IL_PSP_EXT "psp "
#else
	#define IL_PSP_EXT ""
#endif

#ifndef IL_NO_PXR
	#define IL_PXR_EXT "pxr "
#else
	#define IL_PXR_EXT ""
#endif

#ifndef IL_NO_SGI
	#define IL_SGI_EXT "sgi bw rgb rgba "
#else
	#define IL_SGI_EXT ""
#endif

#ifndef IL_NO_TGA
	#define IL_TGA_EXT "tga vda icb vst "
#else
	#define IL_TGA_EXT ""
#endif

#ifndef IL_NO_TIF
	#define IL_TIF_EXT "tif tiff "
#else
	#define IL_TIF_EXT ""
#endif

#ifndef IL_NO_WAL
	#define IL_WAL_EXT "wal "
#else
	#define IL_WAL_EXT ""
#endif

#ifndef IL_NO_XPM
	#define IL_XPM_EXT "xpm "
#else
	#define IL_XPM_EXT ""
#endif



#endif//STATES_H
