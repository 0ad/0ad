//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/19/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_pcd.c
//
// Description: Reads from a Kodak PhotoCD (.pcd) file.
//		Note:  The code here is sloppy - I had to convert it from Pascal,
//				which I've never even attempted to read before...enjoy! =)
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_PCD
#include "il_manip.h"


ILboolean iLoadPcdInternal(ILvoid);

//! Reads a .pcd file
ILboolean ilLoadPcd(const ILstring FileName)
{
	ILHANDLE	PcdFile;
	ILboolean	bPcd = IL_FALSE;

	PcdFile = iopenr(FileName);
	if (PcdFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPcd;
	}

	bPcd = ilLoadPcdF(PcdFile);
	icloser(PcdFile);

	return bPcd;
}


//! Reads an already-opened .pcd file
ILboolean ilLoadPcdF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadPcdInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .pcd file
ILboolean ilLoadPcdL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadPcdInternal();
}


ILvoid YCbCr2RGB(ILubyte Y, ILubyte Cb, ILubyte Cr, ILubyte *r, ILubyte *g, ILubyte *b)
{
	static const ILdouble c11 = 0.0054980*256;
	static const ILdouble c12 = 0.0000000*256;
	static const ILdouble c13 = 0.0051681*256;
	static const ILdouble c21 = 0.0054980*256;
	static const ILdouble c22 =-0.0015446*256;
	static const ILdouble c23 =-0.0026325*256;
	static const ILdouble c31 = 0.0054980*256;
	static const ILdouble c32 = 0.0079533*256;
	static const ILdouble c33 = 0.0000000*256;
	ILint r1, g1, b1;

	r1 = (ILint)(c11*Y + c12*(Cb-156) + c13*(Cr-137));
	g1 = (ILint)(c21*Y + c22*(Cb-156) + c23*(Cr-137));
	b1 = (ILint)(c31*Y + c32*(Cb-156) + c33*(Cr-137));

	if (r1 < 0)
		*r = 0;
	else if (r1 > 255)
		*r = 255;
	else
		*r = r1;

	if (g1 < 0)
		*g = 0;
	else if (g1 > 255)
		*g = 255;
	else
		*g = g1;

	if (b1 < 0)
		*b = 0;
	else if (b1 > 255)
		*b = 255;
	else
		*b = b1;

	return;
}


ILboolean iLoadPcdInternal()
{
	ILubyte	VertOrientation;
	ILuint	Width, Height, i, Total, x, CurPos = 0;
	ILubyte	*Y1=NULL, *Y2=NULL, *CbCr=NULL, r = 0, g = 0, b = 0;
	ILuint	PicNum;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	iseek(72, IL_SEEK_CUR);
	if (iread(&VertOrientation, 1, 1) != 1)
		return IL_FALSE;

	iseek(-72, IL_SEEK_CUR);  // Can't rewind

	PicNum = iGetInt(IL_PCD_PICNUM);

	switch (PicNum)
	{
		case 0:
			iseek(0x02000, IL_SEEK_CUR);
			Width = 192;
			Height = 128;
			break;
		case 1:
			iseek(0x0b800, IL_SEEK_CUR);
			Width = 384;
			Height = 256;
			break;
		case 2:
			iseek(0x30000, IL_SEEK_CUR);
			Width = 768;
			Height = 512;
			break;
		default:
			ilSetError(IL_INVALID_PARAM);
			return IL_FALSE;
	}

	Y1 = (ILubyte*)ialloc(Width);
	Y2 = (ILubyte*)ialloc(Width);
	CbCr = (ILubyte*)ialloc(Width);
	if (Y1 == NULL || Y2 == NULL || CbCr == NULL) {
		ifree(Y1);
		ifree(Y2);
		ifree(CbCr);
		return IL_FALSE;
	}

	if (!ilTexImage(Width, Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	Total = Height >> 1;
	for (i = 0; i < Total; i++) {
		iread(Y1, 1, Width);
		iread(Y2, 1, Width);
		if (iread(CbCr, 1, Width) != Width) {  // Only really need to check the last one.
			ifree(Y1);
			ifree(Y2);
			ifree(CbCr);
			return IL_FALSE;
		}

		for (x = 0; x < Width; x++) {
			YCbCr2RGB(Y1[x], CbCr[x / 2], CbCr[(Width / 2) + (x / 2)], &r, &g, &b);
			iCurImage->Data[CurPos++] = r;
			iCurImage->Data[CurPos++] = g;
			iCurImage->Data[CurPos++] = b;
		}

		for (x = 0; x < Width; x++) {
			YCbCr2RGB(Y2[x], CbCr[x / 2], CbCr[(Width / 2) + (x / 2)], &r, &g, &b);
			iCurImage->Data[CurPos++] = r;
			iCurImage->Data[CurPos++] = g;
			iCurImage->Data[CurPos++] = b;
		}
	}

	ifree(Y1);
	ifree(Y2);
	ifree(CbCr);

	// Not sure how it is...the documentation is hard to understand
	if ((VertOrientation & 0x3F) != 8)
		iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;
	else
		iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	ilFixImage();

	return IL_TRUE;
}


#endif//IL_NO_PCD
