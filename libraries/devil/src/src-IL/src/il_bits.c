//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_bits.c
//
// Description: Implements a file class that reads/writes bits directly.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_bits.h"


// Opens a BITFILE just like fopen opens a FILE.
/*BITFILE *bopen(const char *FileName, const char *Mode)
{
	BITFILE *ToReturn = NULL;

	if (FileName != NULL) {
		ToReturn = (BITFILE*)ialloc(sizeof(BITFILE));
		if (ToReturn != NULL) {
			iopenr((char*)FileName);
			ToReturn->File = iGetFile();
			ToReturn->BitPos = 0;
			ToReturn->ByteBitOff = 8;
			ToReturn->Buff = 0;
		}
	}

	return ToReturn;
}*/


// Converts a FILE to a BITFILE.
BITFILE *bfile(ILHANDLE File)
{
	BITFILE *ToReturn = NULL;

	if (File != NULL) {
		ToReturn = (BITFILE*)ialloc(sizeof(BITFILE));
		if (ToReturn != NULL) {
			ToReturn->File = File;
			ToReturn->BitPos = itell() << 3;
			ToReturn->ByteBitOff = 8;
			ToReturn->Buff = 0;
		}
	}

	return ToReturn;
}


// Closes an open BITFILE and frees memory for it.
ILint bclose(BITFILE *BitFile)
{
	if (BitFile == NULL || BitFile->File == NULL)
		return IL_EOF;

	icloser(BitFile->File);
	ifree(BitFile);

	return 0;
}


// Returns the current bit position of a BITFILE.
ILint btell(BITFILE *BitFile)
{
	return BitFile->BitPos;
}


// Seeks in a BITFILE just like fseek for FILE.
ILint bseek(BITFILE *BitFile, ILuint Offset, ILuint Mode)
{
	ILint KeepPos, Len;

	if (BitFile == NULL || BitFile->File == NULL)
		return 1;

	switch (Mode)
	{
		case IL_SEEK_SET:
			if (iseek(Offset >> 3, Mode)) {
				BitFile->BitPos = Offset;
				BitFile->ByteBitOff = BitFile->BitPos % 8;
			}
			break;
		case IL_SEEK_CUR:
			if (iseek(Offset >> 3, Mode)) {
				BitFile->BitPos += Offset;
				BitFile->ByteBitOff = BitFile->BitPos % 8;
			}
			break;
		case IL_SEEK_END:
			KeepPos = itell();
			iseek(0, IL_SEEK_END);
			Len = itell();
			iseek(0, IL_SEEK_SET);

			if (iseek(Offset >> 3, Mode)) {
				BitFile->BitPos = (Len << 3) + Offset;
				BitFile->ByteBitOff = BitFile->BitPos % 8;
			}

			break;

		default:
			return 1;
	}

	return 0;
}


// hehe, "bread".  It reads data into Buffer from the BITFILE, just like fread for FILE.
ILint bread(ILvoid *Buffer, ILuint Size, ILuint Number, BITFILE *BitFile)
{
	ILuint	BuffPos = 0, Count = Size * Number;

	while (BuffPos < Count) {
		if (BitFile->ByteBitOff < 0 || BitFile->ByteBitOff > 7) {
			BitFile->ByteBitOff = 7;
			if (iread(&BitFile->Buff, 1, 1) != 1)  // Reached eof or error...
				return BuffPos;
		}

		*((ILubyte*)(Buffer) + BuffPos) = (ILubyte)!!(BitFile->Buff & (1 << BitFile->ByteBitOff));

		BuffPos++;
		BitFile->ByteBitOff--;
	}

	return BuffPos;
}


// Not implemented yet.
/*ILint bwrite(ILvoid *Buffer, ILuint Size, ILuint Number, BITFILE *BitFile)
{


	return 0;
}*/
