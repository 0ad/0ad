//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 06/13/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_gif.c
//
// Description: Reads from a Graphics Interchange Format (.gif) file.
//
//  The LZW decompression code is based on code released to the public domain
//    by Javier Arevalo and can be found at
//    http://www.programmersheaven.com/zone10/cat452
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_GIF

#include "il_gif.h"


ILenum	GifType;

//! Checks if the file specified in FileName is a valid Gif file.
ILboolean ilIsValidGif(const ILstring FileName)
{
	ILHANDLE	GifFile;
	ILboolean	bGif = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("gif"))) {
		ilSetError(IL_INVALID_EXTENSION);
		return bGif;
	}

	GifFile = iopenr(FileName);
	if (GifFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bGif;
	}

	bGif = ilIsValidGifF(GifFile);
	icloser(GifFile);

	return bGif;
}


//! Checks if the ILHANDLE contains a valid Gif file at the current position.
ILboolean ilIsValidGifF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iIsValidGif();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Checks if Lump is a valid Gif lump.
ILboolean ilIsValidGifL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iIsValidGif();
}


// Internal function to get the header and check it.
ILboolean iIsValidGif()
{
	char Header[6];
	
	if (iread(Header, 1, 6) != 6)
		return IL_FALSE;
	iseek(-6, IL_SEEK_CUR);

	if (!strnicmp(Header, "GIF87A", 6))
		return IL_TRUE;
	if (!strnicmp(Header, "GIF89A", 6))
		return IL_TRUE;

	return IL_FALSE;
}


//! Reads a Gif file
ILboolean ilLoadGif(const ILstring FileName)
{
	ILHANDLE	GifFile;
	ILboolean	bGif = IL_FALSE;

	GifFile = iopenr(FileName);
	if (GifFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bGif;
	}

	bGif = ilLoadGifF(GifFile);
	icloser(GifFile);

	return bGif;
}


//! Reads an already-opened Gif file
ILboolean ilLoadGifF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadGifInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a Gif
ILboolean ilLoadGifL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadGifInternal();
}


// Internal function used to load the Gif.
ILboolean iLoadGifInternal()
{
	GIFHEAD	Header;
	ILpal	GlobalPal;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	GlobalPal.Palette = NULL;
	GlobalPal.PalSize = 0;
	if (iread(&Header, sizeof(Header), 1) != 1)
		return IL_FALSE;

	UShort(&Header.Width);
	UShort(&Header.Height);

	if (!strnicmp(Header.Sig, "GIF87A", 6)) {
		GifType = GIF87A;
	}
	else if (!strnicmp(Header.Sig, "GIF89A", 6)) {
		GifType = GIF89A;
	}
	else {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ilTexImage(Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;
	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	// Check for a global colour map.
	if (Header.ColourInfo & (1 << 7)) {
		if (!iGetPalette(Header.ColourInfo, &GlobalPal)) {
			return IL_FALSE;
		}
	}

	if (!GetImages(&GlobalPal))
		return IL_FALSE;

	if (GlobalPal.Palette && GlobalPal.PalSize)
		ifree(GlobalPal.Palette);
	GlobalPal.Palette = NULL;
	GlobalPal.PalSize = 0;

	ilFixImage();

	return IL_TRUE;
}


ILboolean iGetPalette(ILubyte Info, ILpal *Pal)
{
	Pal->PalSize = (1 << ((Info & 0x7) + 1)) * 3;
	Pal->PalType = IL_PAL_RGB24;
	Pal->Palette = (ILubyte*)ialloc(Pal->PalSize);
	if (Pal->Palette == NULL)
		return IL_FALSE;
	if (iread(Pal->Palette, 1, Pal->PalSize) != Pal->PalSize) {
		ifree(Pal->Palette);
		Pal->Palette = NULL;
		return IL_FALSE;
	}

	return IL_TRUE;
}


ILboolean GetImages(ILpal *GlobalPal)
{
	IMAGEDESC	ImageDesc;
	GFXCONTROL	Gfx;
	ILboolean	BaseImage = IL_TRUE;
	ILimage		*Image = iCurImage, *TempImage = NULL;
	ILuint		NumImages = 0, i;
	ILubyte		*TempData = NULL;

	Gfx.Used = IL_TRUE;

	while (!ieof()) {
		i = itell();
		if (!SkipExtensions(&Gfx))
			goto error_clean;
		i = itell();

		// Either of these means that we've reached the end of the data stream.
		if (iread(&ImageDesc, sizeof(ImageDesc), 1) != 1) {
			ilGetError();  // Gets rid of the IL_FILE_READ_ERROR that inevitably results.
			break;
		}
		if (ImageDesc.Separator != 0x2C)
			break;
		UShort(&ImageDesc.OffX);
		UShort(&ImageDesc.OffY);
		UShort(&ImageDesc.Width);
		UShort(&ImageDesc.Height);

		if (!BaseImage) {
			NumImages++;
			Image->Next = ilNewImage(iCurImage->Width, iCurImage->Height, 1, 1, 1);
			if (Image->Next == NULL)
				goto error_clean;

			// Each image is based on the previous image, so copy the previous data.
			//if ((Gfx.Packed & 0x1C) >> 2 == 1)
			memcpy(Image->Next->Data, Image->Data, Image->SizeOfData);

			if (ImageDesc.Width != iCurImage->Width || ImageDesc.Height != iCurImage->Height) {
				TempData = (ILubyte*)ialloc(ImageDesc.Width * ImageDesc.Height);
				if (TempData == NULL) {
					goto error_clean;
				}
			}
			else {
				TempData = Image->Next->Data;
			}

			Image = Image->Next;
			Image->Format = IL_COLOUR_INDEX;
			Image->Origin = IL_ORIGIN_UPPER_LEFT;
		}
		else {
			BaseImage = IL_FALSE;
			TempData = iCurImage->Data;
		}

		Image->OffX = ImageDesc.OffX;
		Image->OffY = ImageDesc.OffY;


		// Check to see if the image has its own palette.
		if (ImageDesc.ImageInfo & (1 << 7)) {
			if (!iGetPalette(ImageDesc.ImageInfo, &Image->Pal)) {
				goto error_clean;
			}
		}
		else {
			if (!iCopyPalette(&Image->Pal, GlobalPal)) {
				goto error_clean;
			}
		}


		if (!GifGetData(TempData, Image->SizeOfData)) {
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			goto error_clean;
		}

		if (TempData != Image->Data) {
			for (i = 0; i < ImageDesc.Height; i++) {
				memcpy(&Image->Data[(Image->OffY + i) * Image->Width + Image->OffX],
					&TempData[i * ImageDesc.Width], ImageDesc.Width);
			}

			ifree(TempData);
			TempData = NULL;
		}

		if (ImageDesc.ImageInfo & (1 << 6)) {  // Image is interlaced.
			if (!RemoveInterlace())
				goto error_clean;
		}

		// See if there was a valid graphics control extension.
		if (!Gfx.Used) {
			Gfx.Used = IL_TRUE;
			Image->Duration = Gfx.Delay * 10;  // We want it in milliseconds.

			// See if a transparent colour is defined.
			if (Gfx.Packed & 1) {
				if (!ConvertTransparent(Image, Gfx.Transparent)) {
					goto error_clean;
				}
			}
		}

		i = itell();

		// Terminates each block.
		if (igetc() != 0x00)
			break;
	}

	iCurImage->NumNext = NumImages;
	if (BaseImage)  // Was not able to load any images in...
		return IL_FALSE;

	return IL_TRUE;

error_clean:
	ifree(TempData);
	Image = iCurImage->Next;
	while (Image) {
		TempImage = Image;
		Image = Image->Next;
		ilCloseImage(TempImage);
	}
	
	return IL_FALSE;
}


ILboolean SkipExtensions(GFXCONTROL *Gfx)
{
	ILint	Code;
	ILint	Label;
	ILint	Size;

	// DW (06-03-2002):  Apparently there can be...
	//if (GifType == GIF87A)
	//	return IL_TRUE;  // No extensions in the GIF87a format.

	do {
		Code = igetc();
		if (Code != 0x21) {
			iseek(-1, IL_SEEK_CUR);
			return IL_TRUE;
		}

		Label = igetc();

		switch (Label)
		{
			case 0xF9:
				if (iread(Gfx, sizeof(GFXCONTROL) - sizeof(ILboolean), 1) != 1)
					return IL_FALSE;
				UShort(&Gfx->Delay);
				Gfx->Used = IL_FALSE;

				break;

			/*case 0xFE:
				break;

			case 0x01:
				break;*/

			default:
				do {
					Size = igetc();
					iseek(Size, IL_SEEK_CUR);
				} while (!ieof() && Size != 0);
		}

		// @TODO:  Handle this better.
		if (ieof()) {
			ilSetError(IL_FILE_READ_ERROR);
			return IL_FALSE;
		}
	} while (1);

	return IL_TRUE;
}


#define MAX_CODES 4096

ILint	curr_size, clear, ending, newcodes, top_slot, slot, navail_bytes = 0, nbits_left = 0;
ILubyte	b1;
ILubyte	byte_buff[257];
ILubyte	*pbytes;
ILubyte	*stack;
ILubyte	*suffix;
ILshort	*prefix;

ILuint code_mask[13] =
{
   0L,
   0x0001L, 0x0003L,
   0x0007L, 0x000FL,
   0x001FL, 0x003FL,
   0x007FL, 0x00FFL,
   0x01FFL, 0x03FFL,
   0x07FFL, 0x0FFFL
};


ILint get_next_code(void)
{
	ILint	i;
	ILuint	ret;

	if (!nbits_left) {
		if (navail_bytes <= 0) {
			pbytes = byte_buff;
			navail_bytes = igetc();
			if (navail_bytes) {
				for (i = 0; i < navail_bytes; i++) {
					byte_buff[i] = igetc();
				}
			}
		}
		b1 = *pbytes++;
		nbits_left = 8;
		navail_bytes--;
	}

	ret = b1 >> (8 - nbits_left);
	while (curr_size > nbits_left) {
		if (navail_bytes <= 0) {
			pbytes = byte_buff;
			navail_bytes = igetc();
			if (navail_bytes) {
				for (i = 0; i < navail_bytes; i++) {
					byte_buff[i] = igetc();
				}
			}
		}
		b1 = *pbytes++;
		ret |= b1 << nbits_left;
		nbits_left += 8;
		navail_bytes--;
	}
	nbits_left -= curr_size;

	return (ret & code_mask[curr_size]);
}


ILboolean GifGetData(ILubyte *Data, ILuint ImageSize)
{
	ILubyte	*sp;
	ILint	code, fc, oc;
	ILubyte	size;
	ILint	c;
	ILuint	i = 0;

	size = igetc();
	if (size < 2 || 9 < size) {
		return IL_FALSE;
	}

	stack  = (ILubyte*)ialloc(MAX_CODES + 1);
	suffix = (ILubyte*)ialloc(MAX_CODES + 1);
	prefix = (ILshort*)ialloc(sizeof(*prefix) * (MAX_CODES + 1));
	if (!stack || !suffix || !prefix) {
		ifree(stack);
		ifree(suffix);
		ifree(prefix);
		return IL_FALSE;
	}

	curr_size = size + 1;
	top_slot = 1 << curr_size;
	clear = 1 << size;
	ending = clear + 1;
	slot = newcodes = ending + 1;
	navail_bytes = nbits_left = 0;
	oc = fc = 0;
	sp = stack;

	while ((c = get_next_code()) != ending && i < ImageSize) {
		if (c == clear) {
			curr_size = size + 1;
			slot = newcodes;
			top_slot = 1 << curr_size;
			while ((c = get_next_code()) == clear);
			if (c == ending)
				break;
			if (c >= slot)
				c = 0;
			oc = fc = c;
			//if (i < ImageSize)
				Data[i++] = c;
			//*Data++ = c;
		}
		else {
			code = c;
			if (code >= slot) {
				code = oc;
				*sp++ = fc;
			}
			while (code >= newcodes) {
				*sp++ = suffix[code];
				code = prefix[code];
			}
			*sp++ = (ILbyte)code;
			if (slot < top_slot) {
				fc = code;
				suffix[slot]   = fc;
				prefix[slot++] = oc;
				oc = c;
			}
			if (slot >= top_slot && curr_size < 12) {
				top_slot <<= 1;
				curr_size++;
			}
			while (sp > stack) {
				sp--;
				//if (i < ImageSize)
					Data[i++] = *sp;
				//*Data++ = *sp;
			}
		}
	}

	ifree(stack);
	ifree(suffix);
	ifree(prefix);

	return IL_TRUE;
}


/*From the GIF spec:

  The rows of an Interlaced images are arranged in the following order:

      Group 1 : Every 8th. row, starting with row 0.              (Pass 1)
      Group 2 : Every 8th. row, starting with row 4.              (Pass 2)
      Group 3 : Every 4th. row, starting with row 2.              (Pass 3)
      Group 4 : Every 2nd. row, starting with row 1.              (Pass 4)
*/

ILboolean RemoveInterlace()
{
	ILubyte *NewData;
	ILuint	i, j = 0;

	NewData = (ILubyte*)ialloc(iCurImage->SizeOfData);
	if (NewData == NULL)
		return IL_FALSE;

	for (i = 0; i < iCurImage->Height; i += 8, j++) {
		memcpy(&NewData[i * iCurImage->Bps], &iCurImage->Data[j * iCurImage->Bps], iCurImage->Bps);
	}

	for (i = 4; i < iCurImage->Height; i += 8, j++) {
		memcpy(&NewData[i * iCurImage->Bps], &iCurImage->Data[j * iCurImage->Bps], iCurImage->Bps);
	}

	for (i = 2; i < iCurImage->Height; i += 4, j++) {
		memcpy(&NewData[i * iCurImage->Bps], &iCurImage->Data[j * iCurImage->Bps], iCurImage->Bps);
	}

	for (i = 1; i < iCurImage->Height; i += 2, j++) {
		memcpy(&NewData[i * iCurImage->Bps], &iCurImage->Data[j * iCurImage->Bps], iCurImage->Bps);
	}

	ifree(iCurImage->Data);
	iCurImage->Data = NewData;

	return IL_TRUE;
}


// Assumes that Dest has nothing in it.
ILboolean iCopyPalette(ILpal *Dest, ILpal *Src)
{
	if (Src->Palette == NULL || Src->PalSize == 0)
		return IL_FALSE;

	Dest->Palette = (ILubyte*)ialloc(Src->PalSize);
	if (Dest->Palette == NULL)
		return IL_FALSE;

	memcpy(Dest->Palette, Src->Palette, Src->PalSize);

	Dest->PalSize = Src->PalSize;
	Dest->PalType = Src->PalType;

	return IL_TRUE;
}


// Uses the transparent colour index to make an alpha channel.
ILboolean ConvertTransparent(ILimage *Image, ILubyte TransColour)
{
	ILubyte	*Palette;
	ILuint	i, j;

	if (!Image->Pal.Palette || !Image->Pal.PalSize) {
		ilSetError(IL_INTERNAL_ERROR);
		return IL_FALSE;
	}

	Palette = (ILubyte*)ialloc(Image->Pal.PalSize / 3 * 4);
	if (Palette == NULL)
		return IL_FALSE;

	for (i = 0, j = 0; i < Image->Pal.PalSize; i += 3, j += 4) {
		Palette[j  ] = Image->Pal.Palette[i  ];
		Palette[j+1] = Image->Pal.Palette[i+1];
		Palette[j+2] = Image->Pal.Palette[i+2];
		if (j/4 == TransColour)
			Palette[j+3] = 0x00;
		else
			Palette[j+3] = 0xFF;
	}

	ifree(Image->Pal.Palette);
	Image->Pal.Palette = Palette;
	Image->Pal.PalSize = Image->Pal.PalSize / 3 * 4;
	Image->Pal.PalType = IL_PAL_RGBA32;

	return IL_TRUE;
}

#endif //IL_NO_GIF
