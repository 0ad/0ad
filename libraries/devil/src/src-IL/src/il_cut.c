//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 03/02/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_cut.c
//
// Description: Reads a Dr. Halo .cut file
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_CUT
#include "il_manip.h"
#include "il_pal.h"
#include "il_bits.h"


// Wrap it just in case...
#ifdef _MSC_VER
#pragma pack(push, packed_struct, 1)
#endif
typedef struct CUT_HEAD
{
	ILushort	Width;
	ILushort	Height;
	ILint		Dummy;
} IL_PACKSTRUCT CUT_HEAD;
#ifdef _MSC_VER
#pragma pack(pop,  packed_struct)
#endif

ILboolean iLoadCutInternal();

//! Reads a .cut file
ILboolean ilLoadCut(const ILstring FileName)
{
	ILHANDLE	CutFile;
	ILboolean	bCut = IL_FALSE;

	CutFile = iopenr(FileName);
	if (CutFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bCut;
	}

	bCut = ilLoadCutF(CutFile);
	icloser(CutFile);

	return bCut;
}


//! Reads an already-opened .cut file
ILboolean ilLoadCutF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadCutInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .cut
ILboolean ilLoadCutL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadCutInternal();
}


//	Note:  .Cut support has not been tested yet!
// A .cut can only have 1 bpp.
//	We need to add support for the .pal's PSP outputs with these...
ILboolean iLoadCutInternal()
{
	CUT_HEAD	Header;
	ILuint		Size, i = 0, j;
	ILubyte		Count, Run;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Header.Width = GetLittleShort();
	Header.Height = GetLittleShort();
	Header.Dummy = GetLittleInt();

	if (Header.Width == 0 || Header.Height == 0) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ilTexImage(Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {  // always 1 bpp
		return IL_FALSE;
	}
	iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	Size = Header.Width * Header.Height;

	while (i < Size) {
		Count = igetc();
		if (Count == 0) { // end of row
			igetc();  // Not supposed to be here, but
			igetc();  //  PSP is putting these two bytes here...WHY?!
			continue;
		}
		if (Count & BIT_7) {  // rle-compressed
			ClearBits(Count, BIT_7);
			Run = igetc();
			for (j = 0; j < Count; j++) {
				iCurImage->Data[i++] = Run;
			}
		}
		else {  // run of pixels
			for (j = 0; j < Count; j++) {
				iCurImage->Data[i++] = igetc();
			}
		}
	}

	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;  // Not sure

	/*iCurImage->Pal.Palette = SharedPal.Palette;
	iCurImage->Pal.PalSize = SharedPal.PalSize;
	iCurImage->Pal.PalType = SharedPal.PalType;*/

	ilFixImage();

	return IL_TRUE;
}


ILvoid ilPopToast()
{
	ILstring flipCode = IL_TEXT("#flipCode and www.flipCode.com rule you all.");
	flipCode[0] = flipCode[0];
	return;
}



#endif//IL_NO_CUT
