//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 02/20/2002 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_error.c
//
// Description: Error functions
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"

const ILstring iluErrorStrings[IL_FILE_READ_ERROR - IL_INVALID_ENUM + 1] = {
	IL_TEXT("invalid enumerant"),
    IL_TEXT("out of memory"),
	IL_TEXT("format not supported yet"),
	IL_TEXT("internal error"),
	IL_TEXT("invalid value"),
    IL_TEXT("illegal operation"),
	IL_TEXT("illegal file value"),
	IL_TEXT("invalid file header"),
	IL_TEXT("invalid parameter"),
	IL_TEXT("could not open file"),
	IL_TEXT("invalid extension"),
	IL_TEXT("file already exists"),
	IL_TEXT("out format equivalent"),
	IL_TEXT("stack overflow"),
    IL_TEXT("stack underflow"),
	IL_TEXT("invalid conversion"),
	IL_TEXT("bad dimensions"),
	IL_TEXT("file read error")
};

const ILstring iluLibErrorStrings[IL_LIB_MNG_ERROR - IL_LIB_GIF_ERROR + 1] = {
	IL_TEXT("gif library error"),
	IL_TEXT("jpeg library error"),
	IL_TEXT("png library error"),
	IL_TEXT("tiff library error"),
	IL_TEXT("mng library error")
};


const ILstring ILAPIENTRY iluErrorString(ILenum Error)
{
	if (Error == IL_NO_ERROR) {
		return (const ILstring)"no error";
	}
	if (Error == IL_UNKNOWN_ERROR) {
		return (const ILstring)"unknown error";
	}
	if (Error >= IL_INVALID_ENUM && Error <= IL_FILE_READ_ERROR) {
		return iluErrorStrings[Error - IL_INVALID_ENUM];
	}
	if (Error >= IL_LIB_GIF_ERROR && Error <= IL_LIB_MNG_ERROR) {
		return iluLibErrorStrings[Error - IL_LIB_GIF_ERROR];
	}

	// Siigron: changed this from NULL to "no error"
	return TEXT("no error");
}
