//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/18/2002 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_states.c
//
// Description: The state machine
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"
#include "ilu_states.h"


const ILstring _iluVendor	= IL_TEXT("Abysmal Software");
const ILstring _iluVersion	= IL_TEXT("Developer's Image Library Utilities (ILU) 1.6.7 " __DATE__);


const ILstring ILAPIENTRY iluGetString(ILenum StringName)
{
	switch (StringName)
	{
		case ILU_VENDOR:
			return (const ILstring)_iluVendor;
		//changed 2003-09-04
		case ILU_VERSION_NUM:
			return (const ILstring)_iluVersion;
		default:
			ilSetError(ILU_INVALID_PARAM);
			break;
	}
	return NULL;
}


ILvoid ILAPIENTRY iluGetIntegerv(ILenum Mode, ILint *Param)
{
	switch (Mode)
	{
		case ILU_VERSION_NUM:
			*Param = ILU_VERSION;
			break;

		case ILU_FILTER:
			*Param = iluFilter;
			break;

		default:
			ilSetError(ILU_INVALID_ENUM);
	}
	return;
}


ILint ILAPIENTRY iluGetInteger(ILenum Mode)
{
	ILint Temp;
	Temp = 0;
	iluGetIntegerv(Mode, &Temp);
	return Temp;
}


ILenum iluFilter = ILU_NEAREST;
ILenum iluPlacement = ILU_CENTER;

ILvoid ILAPIENTRY iluImageParameter(ILenum PName, ILenum Param)
{
	switch (PName)
	{
		case ILU_FILTER:
			switch (Param)
			{
				case ILU_NEAREST:
				case ILU_LINEAR:
				case ILU_BILINEAR:
				case ILU_SCALE_BOX:
				case ILU_SCALE_TRIANGLE:
				case ILU_SCALE_BELL:
				case ILU_SCALE_BSPLINE:
				case ILU_SCALE_LANCZOS3:
				case ILU_SCALE_MITCHELL:
					iluFilter = Param;
					break;
				default:
					ilSetError(ILU_INVALID_ENUM);
					return;
			}
			break;

		case ILU_PLACEMENT:
			switch (Param)
			{
				case ILU_LOWER_LEFT:
				case ILU_LOWER_RIGHT:
				case ILU_UPPER_LEFT:
				case ILU_UPPER_RIGHT:
				case ILU_CENTER:
					iluPlacement = Param;
					break;
				default:
					ilSetError(ILU_INVALID_ENUM);
					return;
			}
			break;

		default:
			ilSetError(ILU_INVALID_ENUM);
			return;
	}
	return;
}
