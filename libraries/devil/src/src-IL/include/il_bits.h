//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_bits.h
//
// Description: Implements a file class that reads/writes bits directly.
//
//-----------------------------------------------------------------------------


#ifndef BITS_H
#define BITS_H

#include "il_internal.h"


// Struct for dealing with reading bits from a file
typedef struct BITFILE
{
	ILHANDLE	*File;
	ILuint		BitPos;
	ILint		ByteBitOff;
	ILubyte		Buff;
} BITFILE;

// Functions for reading bits from a file
//BITFILE*	bopen(const char *FileName, const char *Mode);
ILint		bclose(BITFILE *BitFile);
BITFILE*	bfile(ILHANDLE File);
ILint		btell(BITFILE *BitFile);
ILint		bseek(BITFILE *BitFile, ILuint Offset, ILuint Mode);
ILint		bread(ILvoid *Buffer, ILuint Size, ILuint Number, BITFILE *BitFile);
//ILint		bwrite(ILvoid *Buffer, ILuint Size, ILuint Number, BITFILE *BitFile);

// Useful macros for manipulating bits
#define SetBits(var, bits)		(var |= bits)
#define ClearBits(var, bits)	(var &= ~(bits))


#endif//BITS_H
