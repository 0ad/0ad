#ifndef __OGL_H__
#define __OGL_H__

#ifdef __cplusplus
extern "C" {
#endif


// function pointer declarations
#define FUNC(ret, name, params) extern ret (__stdcall *name) params;
#include "glext_funcs.h"
#undef FUNC


#ifdef _WIN32
#ifndef WINGDIAPI
#define WINGDIAPI __declspec(dllimport)
#endif
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
typedef unsigned short wchar_t;	// for glu.h
#endif	// #ifndef _WIN32


#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#endif



#define GL_TEXTURE_IMAGE_SIZE_ARB 0x86A0


extern int max_tex_size;				// [pixels]
extern int tex_units;
extern int max_VAR_elements;			// GF2: 64K; GF3: 1M
extern bool tex_compression_avail;		// S3TC / DXT{1,3,5}
extern int video_mem;					// [MB]; approximate


// check if the extension <ext> is supported by the OpenGL implementation
extern bool oglExtAvail(const char* ext);

// print all OpenGL errors
extern void oglPrintErrors();

// call before using any of the above, and after each mode change
extern void oglInit();


#ifdef __cplusplus
}
#endif

#endif	// #ifndef __OGL_H__
