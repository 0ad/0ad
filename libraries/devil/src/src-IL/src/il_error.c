//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_error.c
//
// Description: The error functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"


#define IL_ERROR_STACK_SIZE 32  // Needed elsewhere?


ILenum	ilErrorNum[IL_ERROR_STACK_SIZE];
ILint	ilErrorPlace = (-1);


// Sets the current error
//	If you go past the stack size for this, it cycles the errors, almost like a LRU algo.
ILAPI ILvoid ILAPIENTRY ilSetError(ILenum Error)
{
	ILuint i;

	ilErrorPlace++;
	if (ilErrorPlace >= IL_ERROR_STACK_SIZE) {
		for (i = 0; i < IL_ERROR_STACK_SIZE - 2; i++) {
			ilErrorNum[i] = ilErrorNum[i+1];
		}
		ilErrorPlace = IL_ERROR_STACK_SIZE - 1;
	}
	ilErrorNum[ilErrorPlace] = Error;

	return;
}


//! Gets the last error on the error stack
ILenum ILAPIENTRY ilGetError(ILvoid)
{
	ILenum ilReturn;

	if (ilErrorPlace >= 0) {
		ilReturn = ilErrorNum[ilErrorPlace];
		ilErrorPlace--;
	}
	else
		ilReturn = IL_NO_ERROR;

	return ilReturn;
}
