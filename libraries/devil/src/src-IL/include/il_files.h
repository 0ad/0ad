//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 10/20/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_files.h
//
// Description: File handling for DevIL
//
//-----------------------------------------------------------------------------

#ifndef FILES_H
#define FILES_H

#if defined (__FILES_C)
#define __FILES_EXTERN
#else
#define __FILES_EXTERN extern
#endif
#include <IL/il.h>


__FILES_EXTERN ILvoid ILAPIENTRY iPreserveReadFuncs(ILvoid);
__FILES_EXTERN ILvoid ILAPIENTRY iRestoreReadFuncs(ILvoid);

__FILES_EXTERN fEofProc		EofProc;
__FILES_EXTERN fGetcProc	GetcProc;
__FILES_EXTERN fReadProc	ReadProc;
__FILES_EXTERN fSeekRProc	SeekRProc;
__FILES_EXTERN fSeekWProc	SeekWProc;
__FILES_EXTERN fTellRProc	TellRProc;
__FILES_EXTERN fTellWProc	TellWProc;
__FILES_EXTERN fPutcProc	PutcProc;
__FILES_EXTERN fWriteProc	WriteProc;

__FILES_EXTERN ILHANDLE		ILAPIENTRY iDefaultOpen(const ILstring FileName);
__FILES_EXTERN ILvoid		ILAPIENTRY iDefaultClose(ILHANDLE Handle);
__FILES_EXTERN ILint		ILAPIENTRY iDefaultGetc(ILHANDLE Handle);
__FILES_EXTERN ILint		ILAPIENTRY iDefaultRead(ILvoid *Buffer, ILuint Size, ILuint Number, ILHANDLE Handle);
__FILES_EXTERN ILint		ILAPIENTRY iDefaultSeekR(ILHANDLE Handle, ILint Offset, ILint Mode);
__FILES_EXTERN ILint		ILAPIENTRY iDefaultSeekW(ILHANDLE Handle, ILint Offset, ILint Mode);
__FILES_EXTERN ILint		ILAPIENTRY iDefaultTellR(ILHANDLE Handle);
__FILES_EXTERN ILint		ILAPIENTRY iDefaultTellW(ILHANDLE Handle);
__FILES_EXTERN ILint		ILAPIENTRY iDefaultPutc(ILubyte Char, ILHANDLE Handle);
__FILES_EXTERN ILint		ILAPIENTRY iDefaultWrite(const ILvoid *Buffer, ILuint Size, ILuint Number, ILHANDLE Handle);

__FILES_EXTERN ILvoid		iSetInputFile(ILHANDLE File);
__FILES_EXTERN ILvoid		iSetInputLump(ILvoid *Lump, ILuint Size);
__FILES_EXTERN ILboolean	(ILAPIENTRY *ieof)(ILvoid);
__FILES_EXTERN ILHANDLE		(ILAPIENTRY *iopenr)(const ILstring);
__FILES_EXTERN ILvoid		(ILAPIENTRY *icloser)(ILHANDLE);
__FILES_EXTERN ILint		(ILAPIENTRY *igetc)(ILvoid);
__FILES_EXTERN ILuint		(ILAPIENTRY *iread)(ILvoid *Buffer, ILuint Size, ILuint Number);
__FILES_EXTERN ILuint		(ILAPIENTRY *iseek)(ILint Offset, ILuint Mode);
__FILES_EXTERN ILuint		(ILAPIENTRY *itell)(ILvoid);

__FILES_EXTERN ILvoid		iSetOutputFile(ILHANDLE File);
__FILES_EXTERN ILvoid		iSetOutputLump(ILvoid *Lump, ILuint Size);
__FILES_EXTERN ILvoid		(ILAPIENTRY *iclosew)(ILHANDLE);
__FILES_EXTERN ILHANDLE		(ILAPIENTRY *iopenw)(const ILstring);
__FILES_EXTERN ILint		(ILAPIENTRY *iputc)(ILubyte Char);
__FILES_EXTERN ILuint		(ILAPIENTRY *iseekw)(ILint Offset, ILuint Mode);
__FILES_EXTERN ILuint		(ILAPIENTRY *itellw)(ILvoid);
__FILES_EXTERN ILint		(ILAPIENTRY *iwrite)(const ILvoid *Buffer, ILuint Size, ILuint Number);
 
__FILES_EXTERN ILHANDLE		ILAPIENTRY iGetFile(ILvoid);
__FILES_EXTERN ILubyte*		ILAPIENTRY iGetLump(ILvoid);

__FILES_EXTERN ILuint		ILAPIENTRY ilprintf(const char *, ...);
__FILES_EXTERN ILvoid		ipad(ILuint NumZeros);

__FILES_EXTERN ILboolean	iPreCache(ILuint Size);
__FILES_EXTERN ILvoid		iUnCache(ILvoid);

#endif//FILES_H
