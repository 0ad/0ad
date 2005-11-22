//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 02/07/2002 <--Y2K Compliant! =]
//
// Filename: src-ILUT/include/ilut_internal.h
//
// Description: Internal stuff for ILUT
//
//-----------------------------------------------------------------------------


#ifndef INTERNAL_H
#define INTERNAL_H

#define _IL_BUILD_LIBRARY
#define _ILU_BUILD_LIBRARY
#define _ILUT_BUILD_LIBRARY

//#define	WIN32_LEAN_AND_MEAN

#include <string.h>

#ifdef _MSC_VER
	#if _MSC_VER > 1000
		#pragma once
		#pragma intrinsic(memcpy)
		#pragma intrinsic(memset)
	#endif // _MSC_VER > 1000
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <IL/ilut.h>
#include <IL/devil_internal_exports.h>

#include <stdlib.h>


extern ILimage *ilutCurImage;


ILvoid	ilutDefaultStates(ILvoid);


// ImageLib Utility Toolkit's OpenGL Functions
#ifdef ILUT_USE_OPENGL
	ILboolean ilutGLInit();
#endif

// ImageLib Utility Toolkit's Win32 Functions
#ifdef ILUT_USE_WIN32
	ILboolean ilutWin32Init();
#endif

// ImageLib Utility Toolkit's Win32 Functions
#ifdef ILUT_USE_DIRECTX8
	ILboolean ilutD3D8Init();
#endif

#ifdef ILUT_USE_DIRECTX9
	ILboolean ilutD3D9Init();
#endif


#endif//INTERNAL_H
