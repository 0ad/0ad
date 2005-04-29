//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 12/04/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/include/ilu_internal.h
//
// Description: Internal stuff for ILU
//
//-----------------------------------------------------------------------------


#ifndef INTERNAL_H
#define INTERNAL_H

#include <string.h>

#ifdef _MSC_VER
	#if _MSC_VER > 1000
		#pragma once
		#pragma intrinsic(memcpy)
		#pragma intrinsic(memset)
		#pragma comment(linker, "/NODEFAULTLIB:libc")
		#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
	#endif // _MSC_VER > 1000
#endif

#define _IL_BUILD_LIBRARY
#define _ILU_BUILD_LIBRARY

// Standard headers
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

// Local headers
#define _IL_BUILD_LIBRARY
#define _ILU_BUILD_LIBRARY

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <IL/ilu.h>
#include <IL/devil_internal_exports.h>


// From DevIL's internal.h:
#ifdef _WIN32_WCE
	#include <windows.h>
	#define IL_TEXT(s) ((char*)TEXT(s))
#elif _WIN32
	#include <windows.h>
	#ifdef _UNICODE
		#define IL_TEXT__(s) L##s
		#define IL_TEXT(s) IL_TEXT__(s)
	#else
		#define IL_TEXT(s) (s)
	#endif
#else
	#ifdef _UNICODE
		#define IL_TEXT__(s) L##s
		#define IL_TEXT(s) IL_TEXT__(s)
		#define TEXT(s) IL_TEXT(s)
	#else
		#define IL_TEXT(s) (s)
		#define TEXT(s) (s)
	#endif
#endif


#ifdef _MSC_VER
	#define INLINE __inline
	#define FINLINE __forceinline
#else
	#define INLINE
	#define FINLINE
#endif


extern ILimage *iluCurImage;


// Useful global variables
extern ILdouble	IL_PI;
extern ILdouble	IL_DEGCONV;


// Internal functions
ILfloat	ilCos(ILfloat Angle);
ILfloat	ilSin(ILfloat Angle);
ILint	ilRound(ILfloat Num);
ILuint	iluScaleAdvanced(ILuint Width, ILuint Height, ILenum Filter);
ILubyte	*iScanFill(ILvoid);


#endif//INTERNAL_H
