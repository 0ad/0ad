//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/28/2001 <--Y2K Compliant! =]
//
// Filename: src-ILUT/include/ilut_states.h
//
// Description: State machine
//
//-----------------------------------------------------------------------------

#ifndef STATES_H
#define STATES_H

#include "ilut_internal.h"


ILboolean ilutAble(ILenum Mode, ILboolean Flag);


#define ILUT_ATTRIB_STACK_MAX 32

ILuint ilutCurrentPos = 0;  // Which position on the stack

//
// Various states
//

typedef struct ILUT_STATES
{

	// ILUT states
	ILboolean	ilutUsePalettes;
	ILboolean	ilutOglConv;
	ILenum		ilutDXTCFormat;

	// GL states
	ILboolean	ilutUseS3TC;
	ILboolean	ilutGenS3TC;
	ILboolean	ilutAutodetectTextureTarget;

	// D3D states
	ILuint		D3DMipLevels;
	ILenum		D3DPool;
	ILint		D3DAlphaKeyColor; // 0x00rrggbb format , -1 for none

} ILUT_STATES;

ILUT_STATES ilutStates[ILUT_ATTRIB_STACK_MAX];


#endif//STATES_H
