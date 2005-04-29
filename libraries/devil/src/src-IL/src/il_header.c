//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 02/19/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_header.c
//
// Description: Generates a C-style header file for the current image.
//
//-----------------------------------------------------------------------------

#ifndef IL_NO_C_HEADER

#include "il_internal.h"


#define MAX_LINE_WIDTH 14 // Just a guess...let's see what's purty!


//! Generates a C-style header file for the current image.
ILboolean ilSaveCHeader(const ILstring FileName, const char *InternalName)
{
	FILE	*HeadFile;
	ILuint	i = 0, j;
	ILimage	*TempImage;
	char	*Name;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Name = iGetString(IL_CHEAD_HEADER_STRING);
	if (Name == NULL)
		Name = (char*)InternalName;

	if (FileName == NULL || Name == NULL ||
#ifndef _UNICODE
		strlen(FileName) < 1 ||	strlen(Name) < 1) {
#else
		wcslen(FileName) < 1 || wcslen(FileName) < 1) {
#endif//_UNICODE
		ilSetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	if (!iCheckExtension(FileName, IL_TEXT("h"))) {
		ilSetError(IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	if (iCurImage->Bpc > 1) {
		TempImage = iConvertImage(iCurImage, iCurImage->Format, IL_UNSIGNED_BYTE);
		if (TempImage == NULL)
			return IL_FALSE;
	}
	else {
		TempImage = iCurImage;
	}

#ifndef _UNICODE
	HeadFile = fopen(FileName, "rb");
#else
	HeadFile = _wfopen(FileName, L"rb");
#endif//_UNICODE

	
	if (HeadFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	fprintf(HeadFile, "//#include <il/il.h>\n");
	fprintf(HeadFile, "// C Image Header:\n\n\n");
	fprintf(HeadFile, "// IMAGE_BPP is in bytes per pixel, *not* bits\n");

	switch (iCurImage->Bpp)
	{
		case 1:
			fprintf(HeadFile, "#define IMAGE_BPP 1\n");
			break;
		case 2:
			fprintf(HeadFile, "#define IMAGE_BPP 2\n");
			break;
		case 3:
			fprintf(HeadFile, "#define IMAGE_BPP 3\n");
			break;
		case 4:
			fprintf(HeadFile, "#define IMAGE_BPP 4\n");
			break;
	}
	fprintf(HeadFile, "#define IMAGE_WIDTH   %d\n", iCurImage->Width);
	fprintf(HeadFile, "#define IMAGE_HEIGHT  %d\n", iCurImage->Height);	
	fprintf(HeadFile, "#define IMAGE_DEPTH   %d\n\n\n", iCurImage->Depth);
	fprintf(HeadFile, "ILubyte %s[] = {\n", Name);


	for (; i < TempImage->SizeOfData; i += MAX_LINE_WIDTH) {
		fprintf(HeadFile, "\t");
		for (j = 0; j < MAX_LINE_WIDTH; j++) {
			if (i + j >= TempImage->SizeOfData - 1) {
				fprintf(HeadFile, "%4d", TempImage->Data[i]);
				break;
			}
			else
				fprintf(HeadFile, "%4d,", TempImage->Data[i]);
		}
		fprintf(HeadFile, "\n");
	}
	if (TempImage != iCurImage)
		ilCloseImage(TempImage);

	fprintf(HeadFile, "};\n");


	if (iCurImage->Pal.Palette && iCurImage->Pal.PalSize && iCurImage->Pal.PalType != IL_PAL_NONE) {
		fprintf(HeadFile, "\n\n");
		fprintf(HeadFile, "#define IMAGE_PALSIZE %d\n\n", iCurImage->Pal.PalSize);
		fprintf(HeadFile, "ILubyte %sPal[] = {\n", Name);
		for (i = 0; i < iCurImage->Pal.PalSize; i += MAX_LINE_WIDTH) {
			fprintf(HeadFile, "\t");
			for (j = 0; j < MAX_LINE_WIDTH; j++) {
				if (i + j >= iCurImage->Pal.PalSize - 1) {
					fprintf(HeadFile, " %4d", iCurImage->Pal.Palette[i]);
					break;
				}
				else
					fprintf(HeadFile, " %4d,", iCurImage->Pal.Palette[i]);
			}
			fprintf(HeadFile, "\n");
		}

		fprintf(HeadFile, "};\n");
	}

	fclose(HeadFile);
	return IL_TRUE;
}



#endif//IL_NO_C_HEADER
