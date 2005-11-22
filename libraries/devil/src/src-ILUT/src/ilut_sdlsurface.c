//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2002 by Denton Woods
// Copyright (C) 2002 Nelson Rush.
// Last modified: 05/18/2002
//
// Filename: src-ILUT/src/ilut_sdlsurface.c
//
// Description:  SDL Surface functions for images
//
//-----------------------------------------------------------------------------


#include "ilut_internal.h"
#ifdef ILUT_USE_SDL
#include <SDL.h>

#ifdef  _MSC_VER
	#pragma comment(lib, "sdl.lib")
#endif//_MSC_VER

int isBigEndian;
int rmask, gmask, bmask, amask;

ILvoid InitSDL()
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	isBigEndian = 1;
	rmask = 0x000000FF;
	gmask = 0x0000FF00;
	bmask = 0x00FF0000;
	amask = 0;
#else
	isBigEndian = 0;
	rmask = 0x00FF0000;
	gmask = 0x0000FF00;
	bmask = 0x000000FF;
	amask = 0;
#endif
	return;
}

//ILboolean ilConvertPal(ILenum DestFormat);

//ILpal *Pal;

// Does not account for converting luminance...
SDL_Surface * ILAPIENTRY ilutConvertToSDLSurface(unsigned int flags)
{
	SDL_Surface *Bitmap;
	ILuint	i = 0, Pad, BppPal;
	ILubyte	*Dest;

	ilutCurImage = ilGetCurImage();
	if (ilutCurImage == NULL) {
		ilSetError(ILUT_ILLEGAL_OPERATION);
		return NULL;
	}

	InitSDL();

	// Should be IL_BGR(A).
	if (ilutCurImage->Format == IL_RGB || ilutCurImage->Format == IL_RGBA) {
		if (!isBigEndian)
			iluSwapColours();
	}

	if (ilutCurImage->Origin == IL_ORIGIN_LOWER_LEFT)
		iluFlipImage();
	if (ilutCurImage->Type > IL_UNSIGNED_BYTE) {}  // Can't do anything about this right now...
	if (ilutCurImage->Type == IL_BYTE) {}  // Can't do anything about this right now...

	Bitmap = SDL_CreateRGBSurface(flags,ilutCurImage->Width,ilutCurImage->Height,ilutCurImage->Bpp * 8,
					rmask,gmask,bmask,amask);

	if (Bitmap == NULL) {
		return IL_FALSE;
	}

	if (SDL_MUSTLOCK(Bitmap))
		SDL_LockSurface(Bitmap);

	Pad = Bitmap->pitch - ilutCurImage->Bps;
	if (Pad == 0) {
		memcpy(Bitmap->pixels, ilutCurImage->Data, ilutCurImage->SizeOfData);
	}
	else {  // Must pad the lines on some images.
		Dest = Bitmap->pixels;
		for (i = 0; i < ilutCurImage->Height; i++) {
			memcpy(Dest, ilutCurImage->Data + i * ilutCurImage->Bps, ilutCurImage->Bps);
			imemclear(Dest + ilutCurImage->Bps, Pad);
			Dest += Bitmap->pitch;
		}
	}

	if (SDL_MUSTLOCK(Bitmap))
		SDL_UnlockSurface(Bitmap);

	if (ilutCurImage->Bpp == 1) {
		BppPal = ilGetBppPal(ilutCurImage->Pal.PalType);
		switch (ilutCurImage->Pal.PalType)
		{
			case IL_PAL_RGB24:
			case IL_PAL_RGB32:
			case IL_PAL_RGBA32:
				for (i = 0; i < ilutCurImage->Pal.PalSize / BppPal; i++) {
					(Bitmap->format)->palette->colors[i].r = ilutCurImage->Pal.Palette[i*BppPal+0];
					(Bitmap->format)->palette->colors[i].g = ilutCurImage->Pal.Palette[i*BppPal+1];
					(Bitmap->format)->palette->colors[i].b = ilutCurImage->Pal.Palette[i*BppPal+2];
					(Bitmap->format)->palette->colors[i].unused = 255;
				}
				break;
			case IL_PAL_BGR24:
			case IL_PAL_BGR32:
			case IL_PAL_BGRA32:
				for (i = 0; i < ilutCurImage->Pal.PalSize / BppPal; i++) {
					(Bitmap->format)->palette->colors[i].b = ilutCurImage->Pal.Palette[i*BppPal+0];
					(Bitmap->format)->palette->colors[i].g = ilutCurImage->Pal.Palette[i*BppPal+1];
					(Bitmap->format)->palette->colors[i].r = ilutCurImage->Pal.Palette[i*BppPal+2];
					(Bitmap->format)->palette->colors[i].unused = 255;
				}
				break;
			default:
				ilSetError(IL_INTERNAL_ERROR);  // Do anything else?
		}
	}

	return Bitmap;
}


#ifndef _WIN32_WCE
SDL_Surface* ILAPIENTRY ilutSDLSurfaceLoadImage(const ILstring FileName)
{
	SDL_Surface *Surface;

	iBindImageTemp();
	if (!ilLoadImage(FileName)) {
		return NULL;
	}

	Surface = ilutConvertToSDLSurface(SDL_SWSURFACE);

	return Surface;
}
#endif//_WIN32_WCE


// Unfinished
ILboolean ILAPIENTRY ilutSDLSurfaceFromBitmap(SDL_Surface *Bitmap)
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

	return IL_TRUE;
}


#endif//ILUT_USE_SDL
