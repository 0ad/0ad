//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2001-2002 by Denton Woods
// Last modified: 06/13/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_icon.c
//
// Description: Reads from a Windows icon (.ico) file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_ICO
#include "il_icon.h"


//! Reads an icon file.
ILboolean ilLoadIcon(const ILstring FileName)
{
	ILHANDLE	IconFile;
	ILboolean	bIcon = IL_FALSE;

	IconFile = iopenr(FileName);
	if (IconFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bIcon;
	}

	bIcon = ilLoadIconF(IconFile);
	icloser(IconFile);

	return bIcon;
}


//! Reads an already-opened icon file.
ILboolean ilLoadIconF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadIconInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains an icon.
ILboolean ilLoadIconL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadIconInternal();
}


// Internal function used to load the icon.
ILboolean iLoadIconInternal()
{
	ICODIR		IconDir;
	ICODIRENTRY	*DirEntries = NULL;
	ICOIMAGE	*IconImages = NULL;
	ILimage		*Image = NULL;
	ILint		i;
	ILuint		Size, PadSize, j, k, l, m, /*NumImages = 0,*/ CurAndByte, PadCount, AndBytes;
	ILboolean	BaseCreated = IL_FALSE;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iread(&IconDir, sizeof(ICODIR), 1) != 1)
		goto file_read_error;
	Short(&IconDir.Count);
	Short(&IconDir.Type);

	DirEntries = (ICODIRENTRY*)ialloc(sizeof(ICODIRENTRY) * IconDir.Count);
	IconImages = (ICOIMAGE*)ialloc(sizeof(ICOIMAGE) * IconDir.Count);
	if (DirEntries == NULL || IconImages == NULL) {
		ifree(DirEntries);
		ifree(IconImages);
		return IL_FALSE;
	}

	// Make it easier for file_read_error.
	for (i = 0; i < IconDir.Count; i++)
		imemclear(&IconImages[i], sizeof(ICOIMAGE));
	if (iread(DirEntries, sizeof(ICODIRENTRY), IconDir.Count) != (ILuint)IconDir.Count)
		goto file_read_error;

	for (i = 0; i < IconDir.Count; i++) {
		Short(&DirEntries[i].Planes);
		Short(&DirEntries[i].Bpp);
		UInt(&DirEntries[i].SizeOfData);
		UInt(&DirEntries[i].Offset);

		iseek(DirEntries[i].Offset, IL_SEEK_SET);
		if (iread(&IconImages[i].Head, sizeof(INFOHEAD), 1) != 1)
			goto file_read_error;

		Int(&IconImages[i].Head.Size);
		Int(&IconImages[i].Head.Width);
		Int(&IconImages[i].Head.Height);
		Short(&IconImages[i].Head.Planes);
		Short(&IconImages[i].Head.BitCount);
		Int(&IconImages[i].Head.Compression);
		Int(&IconImages[i].Head.SizeImage);
		Int(&IconImages[i].Head.XPixPerMeter);
		Int(&IconImages[i].Head.YPixPerMeter);
		Int(&IconImages[i].Head.ColourUsed);
		Int(&IconImages[i].Head.ColourImportant);

		if (IconImages[i].Head.BitCount < 8) {
			if (IconImages[i].Head.ColourUsed == 0) {
				switch (IconImages[i].Head.BitCount)
				{
					case 1:
						IconImages[i].Head.ColourUsed = 2;
						break;
					case 4:
						IconImages[i].Head.ColourUsed = 16;
						break;
				}
			}
			IconImages[i].Pal = (ILubyte*)ialloc(IconImages[i].Head.ColourUsed * 4);
			if (IconImages[i].Pal == NULL)
				goto file_read_error;  // @TODO: Rename the label.
			if (iread(IconImages[i].Pal, IconImages[i].Head.ColourUsed * 4, 1) != 1)
				goto file_read_error;
		}
		else if (IconImages[i].Head.BitCount == 8) {
			IconImages[i].Pal = (ILubyte*)ialloc(256 * 4);
			if (IconImages[i].Pal == NULL)
				goto file_read_error;
			if (iread(IconImages[i].Pal, 1, 256 * 4) != 256*4)
				goto file_read_error;
		}
		else {
			IconImages[i].Pal = NULL;
		}

		PadSize = (IconImages[i].Head.Width >> 3) % 4;  // Has to be DWORD-aligned.

		if (IconImages[i].Head.BitCount == 1) {
			Size = ((IconImages[i].Head.Width >> 3) + PadSize) * (IconImages[i].Head.Height / 2);
		}
		else {
			Size = (IconImages[i].Head.Width * (IconImages[i].Head.Height / 2) * IconImages[i].Head.BitCount) >> 3;
		}
		IconImages[i].Data = (ILubyte*)ialloc(Size);
		if (IconImages[i].Data == NULL)
			goto file_read_error;
		if (iread(IconImages[i].Data, 1, Size) != Size)
			goto file_read_error;

		//Size = (IconImages[i].Head.Width * (IconImages[i].Head.Height / 2)) >> 3;  // 1 bpp
		Size = ((IconImages[i].Head.Width >> 3) + PadSize) * (IconImages[i].Head.Height / 2);
		IconImages[i].AND = (ILubyte*)ialloc(Size);
		if (IconImages[i].AND == NULL)
			goto file_read_error;
		if (iread(IconImages[i].AND, 1, Size) != Size)
			goto file_read_error;
	}


	for (i = 0; i < IconDir.Count; i++) {
		if (IconImages[i].Head.BitCount != 1 && IconImages[i].Head.BitCount != 4 &&
			IconImages[i].Head.BitCount != 8 && IconImages[i].Head.BitCount != 24)
			continue;

		if (!BaseCreated) {
			ilTexImage(IconImages[i].Head.Width, IconImages[i].Head.Height / 2, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL);
			iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;
			Image = iCurImage;
			BaseCreated = IL_TRUE;
			iCurImage->NumNext = 0;
		}
		else {
			Image->Next = ilNewImage(IconImages[i].Head.Width, IconImages[i].Head.Height / 2, 1, 4, 1);
			Image = Image->Next;
			Image->Format = IL_BGRA;
			iCurImage->NumNext++;
		}
		Image->Type = IL_UNSIGNED_BYTE;

		j = 0;  k = 0;  l = 128;  CurAndByte = 0;  PadCount = 0;
		PadSize = (IconImages[i].Head.Width >> 3) % 4;
		AndBytes = (IconImages[i].Head.Width >> 3);

		if (IconImages[i].Head.BitCount == 1) {
			for (; j < Image->SizeOfData; k++) {
				for (m = 128; m; m >>= 1) {
					Image->Data[j] = IconImages[i].Pal[!!(IconImages[i].Data[k] & m) * 4];
					Image->Data[j+1] = IconImages[i].Pal[!!(IconImages[i].Data[k] & m) * 4 + 1];
					Image->Data[j+2] = IconImages[i].Pal[!!(IconImages[i].Data[k] & m) * 4 + 2];
					Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
					j += 4;
					l >>= 1;
				}
				if (l == 0) {
					l = 128;
					CurAndByte++;
					if (++PadCount == AndBytes) {
						CurAndByte += PadSize;
						k += PadSize;
						PadCount = 0;
					}
				}
			}
		}
		else if (IconImages[i].Head.BitCount == 4) {
			for (; j < Image->SizeOfData; j += 8, k++) {
				Image->Data[j] = IconImages[i].Pal[((IconImages[i].Data[k] & 0xF0) >> 4) * 4];
				Image->Data[j+1] = IconImages[i].Pal[((IconImages[i].Data[k] & 0xF0) >> 4) * 4 + 1];
				Image->Data[j+2] = IconImages[i].Pal[((IconImages[i].Data[k] & 0xF0) >> 4) * 4 + 2];
				Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
				l >>= 1;
				Image->Data[j+4] = IconImages[i].Pal[(IconImages[i].Data[k] & 0x0F) * 4];
				Image->Data[j+5] = IconImages[i].Pal[(IconImages[i].Data[k] & 0x0F) * 4 + 1];
				Image->Data[j+6] = IconImages[i].Pal[(IconImages[i].Data[k] & 0x0F) * 4 + 2];
				Image->Data[j+7] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
				l >>= 1;
				if (l == 0) {
					l = 128;
					CurAndByte++;
					if (++PadCount == AndBytes) {
						CurAndByte += PadSize;
						PadCount = 0;
					}
				}
			}
		}
		else if (IconImages[i].Head.BitCount == 8) {
			for (; j < Image->SizeOfData; j += 4, k++) {
				Image->Data[j] = IconImages[i].Pal[IconImages[i].Data[k] * 4];
				Image->Data[j+1] = IconImages[i].Pal[IconImages[i].Data[k] * 4 + 1];
				Image->Data[j+2] = IconImages[i].Pal[IconImages[i].Data[k] * 4 + 2];
				Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
				l >>= 1;
				if (l == 0) {
					l = 128;
					CurAndByte++;
					if (++PadCount == AndBytes) {
						CurAndByte += PadSize;
						PadCount = 0;
					}
				}
			}
		}
		else if (IconImages[i].Head.BitCount == 24) {
			for (; j < Image->SizeOfData; j += 4, k += 3) {
				Image->Data[j] = IconImages[i].Data[k];
				Image->Data[j+1] = IconImages[i].Data[k+1];
				Image->Data[j+2] = IconImages[i].Data[k+2];
				Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
				l >>= 1;
				if (l == 0) {
					l = 128;
					CurAndByte++;
					if (++PadCount == AndBytes) {
						CurAndByte += PadSize;
						PadCount = 0;
					}
				}
			}
		}
	}


	for (i = 0; i < IconDir.Count; i++) {
		ifree(IconImages[i].Pal);
		ifree(IconImages[i].Data);
		ifree(IconImages[i].AND);
	}
	ifree(IconImages);
	ifree(DirEntries);

	ilFixImage();

	return IL_TRUE;

file_read_error:
	if (IconImages) {
		for (i = 0; i < IconDir.Count; i++) {
			if (IconImages[i].Pal)
				ifree(IconImages[i].Pal);
			if (IconImages[i].Data)
				ifree(IconImages[i].Data);
			if (IconImages[i].AND)
				ifree(IconImages[i].AND);
		}
		ifree(IconImages);
	}
	if (DirEntries)
		ifree(DirEntries);
	return IL_FALSE;
}


#endif
