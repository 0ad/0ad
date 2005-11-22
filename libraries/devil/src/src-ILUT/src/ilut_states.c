//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/18/2002 <--Y2K Compliant! =]
//
// Filename: src-ILUT/src/ilut_states.c
//
// Description: The state machine
//
//-----------------------------------------------------------------------------


#include "ilut_internal.h"
#include "ilut_states.h"
#ifdef ILUT_USE_OPENGL
	#include "ilut_opengl.h"
#endif


const char *_ilutVendor		= "Abysmal Software";
const char *_ilutVersion	= "Developer's Image Library Utility Toolkit (ILUT) 1.6.7 " __DATE__;


// Set all states to their defaults
ILvoid ilutDefaultStates()
{
	ilutStates[ilutCurrentPos].ilutUsePalettes = IL_FALSE;
	ilutStates[ilutCurrentPos].ilutOglConv = IL_FALSE;  // IL_TRUE ?
	ilutStates[ilutCurrentPos].ilutDXTCFormat = 0;
	ilutStates[ilutCurrentPos].ilutUseS3TC = IL_FALSE;
	ilutStates[ilutCurrentPos].ilutGenS3TC = IL_FALSE;
	ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget = IL_FALSE;
	ilutStates[ilutCurrentPos].D3DMipLevels = 0;
	ilutStates[ilutCurrentPos].D3DPool = 0;
	ilutStates[ilutCurrentPos].D3DAlphaKeyColor = -1;
}


ILvoid ILAPIENTRY ilutD3D8MipFunc(ILuint NumLevels)
{
	ilutStates[ilutCurrentPos].D3DMipLevels = NumLevels;
	return;
}


const ILstring ILAPIENTRY ilutGetString(ILenum StringName)
{
	switch (StringName)
	{
		case ILUT_VENDOR:
			return (const ILstring)_ilutVendor;
		//changed 2003-09-04
		case ILUT_VERSION_NUM:
			return (const ILstring)_ilutVersion;
		default:
			ilSetError(ILUT_INVALID_PARAM);
			break;
	}
	return NULL;
}


ILboolean ILAPIENTRY ilutEnable(ILenum Mode)
{
	return ilutAble(Mode, IL_TRUE);
}


ILboolean ILAPIENTRY ilutDisable(ILenum Mode)
{
	return ilutAble(Mode, IL_FALSE);
}


ILboolean ilutAble(ILenum Mode, ILboolean Flag)
{
	switch (Mode)
	{
		case ILUT_PALETTE_MODE:
			ilutStates[ilutCurrentPos].ilutUsePalettes = Flag;
			break;

		case ILUT_OPENGL_CONV:
			ilutStates[ilutCurrentPos].ilutOglConv = Flag;
			break;

		case ILUT_GL_USE_S3TC:
			ilutStates[ilutCurrentPos].ilutUseS3TC = Flag;
			break;

		case ILUT_GL_GEN_S3TC:
			ilutStates[ilutCurrentPos].ilutGenS3TC = Flag;
			break;

		case ILUT_GL_AUTODETECT_TEXTURE_TARGET:
			ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget = Flag;
			break;


		default:
			ilSetError(ILUT_INVALID_ENUM);
			return IL_FALSE;
	}

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilutIsEnabled(ILenum Mode)
{
	switch (Mode)
	{
		case ILUT_PALETTE_MODE:
			return ilutStates[ilutCurrentPos].ilutUsePalettes;

		case ILUT_OPENGL_CONV:
			return ilutStates[ilutCurrentPos].ilutOglConv;

		case ILUT_GL_USE_S3TC:
			return ilutStates[ilutCurrentPos].ilutUseS3TC;

		case ILUT_GL_GEN_S3TC:
			return ilutStates[ilutCurrentPos].ilutGenS3TC;

		case ILUT_GL_AUTODETECT_TEXTURE_TARGET:
			return ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget;


		default:
			ilSetError(ILUT_INVALID_ENUM);
	}

	return IL_FALSE;
}


ILboolean ILAPIENTRY ilutIsDisabled(ILenum Mode)
{
	return !ilutIsEnabled(Mode);
}


ILvoid ILAPIENTRY ilutGetBooleanv(ILenum Mode, ILboolean *Param)
{
	switch (Mode)
	{
		case ILUT_PALETTE_MODE:
			*Param = ilutStates[ilutCurrentPos].ilutUsePalettes;
			break;

		case ILUT_OPENGL_CONV:
			*Param = ilutStates[ilutCurrentPos].ilutOglConv;
			break;

		case ILUT_GL_USE_S3TC:
			*Param = ilutStates[ilutCurrentPos].ilutUseS3TC;
			break;

		case ILUT_GL_GEN_S3TC:
			*Param = ilutStates[ilutCurrentPos].ilutGenS3TC;
			break;

		case ILUT_GL_AUTODETECT_TEXTURE_TARGET:
			*Param = ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget;

		default:
			ilSetError(ILUT_INVALID_ENUM);
	}
	return;
}


ILboolean ILAPIENTRY ilutGetBoolean(ILenum Mode)
{
	ILboolean Temp = IL_FALSE;
	ilutGetBooleanv(Mode, &Temp);
	return Temp;
}


ILvoid ILAPIENTRY ilutGetIntegerv(ILenum Mode, ILint *Param)
{
	switch (Mode)
	{
		/*case IL_ORIGIN_MODE:
			*Param = ilutStates[ilutCurrentPos].ilutOriginMode;
			break;*/
		case ILUT_VERSION_NUM:
			*Param = ILUT_VERSION;
			break;
		case ILUT_PALETTE_MODE:
			*Param = ilutStates[ilutCurrentPos].ilutUsePalettes;
			break;
		case ILUT_OPENGL_CONV:
			*Param = ilutStates[ilutCurrentPos].ilutOglConv;
			break;
		case ILUT_GL_USE_S3TC:
			*Param = ilutStates[ilutCurrentPos].ilutUseS3TC;
			break;
		case ILUT_GL_GEN_S3TC:
			*Param = ilutStates[ilutCurrentPos].ilutUseS3TC;
			break;
		case ILUT_S3TC_FORMAT:
			*Param = ilutStates[ilutCurrentPos].ilutDXTCFormat;
			break;
		case ILUT_GL_AUTODETECT_TEXTURE_TARGET:
			*Param = ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget;
			break;
		case ILUT_D3D_MIPLEVELS:
			*Param = ilutStates[ilutCurrentPos].D3DMipLevels;
			break;
		case ILUT_D3D_ALPHA_KEY_COLOR:
			*Param = ilutStates[ilutCurrentPos].D3DAlphaKeyColor;
			break;
		case ILUT_D3D_POOL:
			*Param = ilutStates[ilutCurrentPos].D3DPool;
			break;

		default:
			ilSetError(ILUT_INVALID_ENUM);
	}
	return;
}


ILint ILAPIENTRY ilutGetInteger(ILenum Mode)
{
	ILint Temp = 0;
	ilutGetIntegerv(Mode, &Temp);
	return Temp;
}


ILvoid ILAPIENTRY ilutSetInteger(ILenum Mode, ILint Param)
{
	switch (Mode)
	{
		case ILUT_S3TC_FORMAT:
			if (Param >= IL_DXT1 && Param <= IL_DXT5) {
				ilutStates[ilutCurrentPos].ilutDXTCFormat = Param;
				return;
			}

//#ifdef ILUT_USE_OPENGL
		case ILUT_MAXTEX_WIDTH:
			iGLSetMaxW(Param);
			return;
		case ILUT_MAXTEX_HEIGHT:
			iGLSetMaxH(Param);
			return;
		case ILUT_MAXTEX_DEPTH:

			break;
		case ILUT_GL_USE_S3TC:
			if (Param == IL_TRUE || Param == IL_FALSE) {
				ilutStates[ilutCurrentPos].ilutUseS3TC = Param;
				return;
			}
			break;
		case ILUT_GL_GEN_S3TC:
			if (Param == IL_TRUE || Param == IL_FALSE) {
				ilutStates[ilutCurrentPos].ilutGenS3TC = Param;
				return;
			}
			break;
		case ILUT_GL_AUTODETECT_TEXTURE_TARGET:
			if (Param == IL_TRUE || Param == IL_FALSE) {
				ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget = Param;
				return;
			}
			break;
//#endif//ILUT_USE_OPENGL

//#ifdef ILUT_USE_DIRECTX8
		case ILUT_D3D_MIPLEVELS:
			if (Param >= 0) {
				ilutStates[ilutCurrentPos].D3DMipLevels = Param;
				return;
			}
			break;

		case ILUT_D3D_ALPHA_KEY_COLOR:
				ilutStates[ilutCurrentPos].D3DAlphaKeyColor = Param;
				return;
			break;

		case ILUT_D3D_POOL:
			if (Param >= 0 && Param <= 2) {
				ilutStates[ilutCurrentPos].D3DPool = Param;
				return;
			}
			break;
//#endif//ILUT_USE_DIRECTX8

		default:
			ilSetError(ILUT_INVALID_ENUM);
	}

	ilSetError(IL_INVALID_PARAM);  // Parameter not in valid bounds.
	return;
}


ILvoid ILAPIENTRY ilutPushAttrib(ILuint Bits)
{
	// Should we check here to see if ilCurrentPos is negative?

	if (ilutCurrentPos >= ILUT_ATTRIB_STACK_MAX - 1) {
		ilutCurrentPos = ILUT_ATTRIB_STACK_MAX - 1;
		ilSetError(ILUT_STACK_OVERFLOW);
		return;
	}

	ilutCurrentPos++;

	//memcpy(&ilutStates[ilutCurrentPos], &ilutStates[ilutCurrentPos - 1], sizeof(ILUT_STATES));

	if (Bits & ILUT_OPENGL_BIT) {
		ilutStates[ilutCurrentPos].ilutUsePalettes = ilutStates[ilutCurrentPos-1].ilutUsePalettes;
		ilutStates[ilutCurrentPos].ilutOglConv = ilutStates[ilutCurrentPos-1].ilutOglConv;
	}
	if (Bits & ILUT_D3D_BIT) {
		ilutStates[ilutCurrentPos].D3DMipLevels = ilutStates[ilutCurrentPos-1].D3DMipLevels;
		ilutStates[ilutCurrentPos].D3DAlphaKeyColor = ilutStates[ilutCurrentPos-1].D3DAlphaKeyColor;
	}

	return;
}


ILvoid ILAPIENTRY ilutPopAttrib()
{
	if (ilutCurrentPos <= 0) {
		ilutCurrentPos = 0;
		ilSetError(ILUT_STACK_UNDERFLOW);
		return;
	}

	// Should we check here to see if ilutCurrentPos is too large?
	ilutCurrentPos--;

	return;
}


ILboolean ILAPIENTRY ilutRenderer(ILenum Renderer)
{
	if (Renderer > ILUT_WIN32) {
		ilSetError(ILUT_INVALID_VALUE);
		return IL_FALSE;
	}

	switch (Renderer)
	{
		#ifdef ILUT_USE_OPENGL
		case ILUT_OPENGL:
			return ilutGLInit();
		#endif

		#ifdef ILUT_USE_WIN32
		case ILUT_WIN32:
			return ilutWin32Init();
		#endif

		#ifdef ILUT_USE_DIRECTX8
		case ILUT_DIRECT3D8:
			return ilutD3D8Init();
		#endif


		default:
			ilSetError(ILUT_NOT_SUPPORTED);
	}

	return IL_FALSE;
}
