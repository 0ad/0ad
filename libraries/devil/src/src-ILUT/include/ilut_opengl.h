//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2002 <--Y2K Compliant! =]
//
// Filename: src-ILUT/include/ilut_opengl.c
//
// Description: OpenGL functions for images
//
//-----------------------------------------------------------------------------

#ifndef ILUT_OPENGL_H
#define ILUT_OPENGL_H


#include "ilut_internal.h"


#ifndef min
#define min(a, b)	(((a) < (b)) ? (a) : (b))
#endif


ILvoid iGLSetMaxW(ILuint Width);
ILvoid iGLSetMaxH(ILuint Height);

ILenum		ilutGLFormat(ILenum, ILubyte);
ILimage*	MakeGLCompliant(ILimage *Src);
ILboolean	IsExtensionSupported(const char *extension);


#ifdef _MSC_VER
	typedef void (ILAPIENTRY * ILGLCOMPRESSEDTEXIMAGE2DARBPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
#endif


#endif//ILUT_OPENGL_H
