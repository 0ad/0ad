//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_mdl.h
//
// Description: Reads a Half-Life model file.
//
//-----------------------------------------------------------------------------


#ifndef MD2_H
#define MD2_H

#include "il_internal.h"

typedef struct TEX_HEAD
{
	char	Name[64];
	ILuint	Flags;
	ILuint	Width;
	ILuint	Height;
	ILuint	Offset;
} TEX_HEAD;

#endif//MD2_H
