//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000 by Denton Woods
// Last modified: 11/23/2000 <--Y2K Compliant! =]
//
// Filename: openilut/allegro.c
//
// Description:  Allegro functions for images
//
//-----------------------------------------------------------------------------


#include "ilut_internal.h"
#ifdef ILUT_USE_ALLEGRO
#include "ilut_allegro.h"

ILboolean ilConvertPal(ILenum DestFormat);

// Does not account for converting luminance...
BITMAP* ILAPIENTRY ilutConvertToAlleg(PALETTE Pal)
{
	BITMAP *Bitmap;
	ILimage *TempImage;
	ILuint i = 0, j = 0;

	ilutCurImage = ilGetCurImage();

	if (ilutCurImage == NULL) {
		ilSetError(ILUT_ILLEGAL_OPERATION);
		return NULL;
	}

	// Should be IL_BGR(A), but Djgpp screws up somewhere along the line.
	if (ilutCurImage->Format == IL_RGB || ilutCurImage->Format == IL_RGBA) {
		iluSwapColours();
	}

	if (ilutCurImage->Origin == IL_ORIGIN_LOWER_LEFT)
		iluFlipImage();
	if (ilutCurImage->Type > IL_UNSIGNED_BYTE) {}  // Can't do anything about this right now...
	if (ilutCurImage->Type == IL_BYTE) {}  // Can't do anything about this right now...

	Bitmap = create_bitmap_ex(ilutCurImage->Bpp * 8, ilutCurImage->Width, ilutCurImage->Height);
	if (Bitmap == NULL) {
		return IL_FALSE;
	}
	memcpy(Bitmap->dat, ilutCurImage->Data, ilutCurImage->SizeOfData);

	// Should we make this toggleable?
	if (ilutCurImage->Bpp == 8 && ilutCurImage->Pal.PalType != IL_PAL_NONE) {
		// Use the image's palette if there is one

		// ilConvertPal is destructive to the original image
		// @TODO:  Use new ilCopyPal!!!
		TempImage = ilNewImage(ilutCurImage->Width, ilutCurImage->Height, ilutCurImage->Depth, ilutCurImage->Bpp, 1);
		ilCopyImageAttr(TempImage, ilutCurImage);
		ilSetCurImage(TempImage);

		if (!ilConvertPal(IL_PAL_RGB24)) {
			destroy_bitmap(Bitmap);
			ilSetError(ILUT_ILLEGAL_OPERATION);
			return NULL;
		}

		for (; i < ilutCurImage->Pal.PalSize && i < 768; i += 3, j++) {
			Pal[j].r = TempImage->Pal.Palette[i+0];
			Pal[j].g = TempImage->Pal.Palette[i+1];
			Pal[j].b = TempImage->Pal.Palette[i+2];
			Pal[j].filler = 255;
		}

		ilCloseImage(TempImage);
		ilSetCurImage(ilutCurImage);
	}

	return Bitmap;
}


#ifndef _WIN32_WCE
BITMAP* ILAPIENTRY ilutAllegLoadImage(const ILstring FileName)
{
	ILuint	ImgId;
	PALETTE	Pal;

	ilGenImages(1, &ImgId);
	ilBindImage(ImgId);
	if (!ilLoadImage(FileName)) {
		ilDeleteImages(1, &ImgId);
		return 0;
	}

	ilDeleteImages(1, &ImgId);

	return ilutConvertToAlleg(Pal);
}
#endif//_WIN32_WCE


// Unfinished
ILboolean ILAPIENTRY ilutAllegFromBitmap(BITMAP *Bitmap)
{
	ilutCurImage = ilGetCurImage();
	if (ilutCurImage == NULL) {
		ilSetError(ILUT_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Bitmap == NULL || Bitmap->w == 0 || Bitmap->h == 0) {
		ilSetError(ILUT_INVALID_PARAM);
		return IL_FALSE;
	}

	if (!ilTexImage(Bitmap->w, Bitmap->h, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	ilutCurImage->Origin = IL_ORIGIN_LOWER_LEFT;  // I have no idea.

	return IL_TRUE;
}


#endif//ILUT_USE_ALLEGRO
