//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 08/11/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/include/ilu_manip.h
//
// Description: Generates mipmaps for the current image
//
//-----------------------------------------------------------------------------


#ifndef MIPMAP_H
#define MIPMAP_H

#include "ilu_internal.h"

ILboolean	iBuild1DMipmaps_(ILuint Width);
ILboolean	iBuild1DMipmapsVertical_(ILuint Height);
ILboolean	iBuild2DMipmaps_(ILuint Width, ILuint Height);
ILboolean	iBuild3DMipmapsVertical_(ILuint Height, ILuint Depth);
ILboolean	iBuild3DMipmapsHorizontal_(ILuint Width, ILuint Depth);
ILboolean	iBuild3DMipmaps_(ILuint Width, ILuint Height, ILuint Depth);

ILimage		*CurMipMap = NULL;  // 8-11-2001

#endif//MIPMAP_H
