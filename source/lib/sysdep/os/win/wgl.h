/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Windows definitions required for GL/gl.h
 */

// RAGE! Win32 OpenGL headers are full of crap we have to emulate
// (must not include windows.h)

#ifndef WGL_HEADER_NEEDED
#error "wgl.h: why is this included from anywhere but ogl.h?"
#endif


#ifndef WINGDIAPI
#define WINGDIAPI __declspec(dllimport)
#endif
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#ifndef WINAPI
#define WINAPI __stdcall
#endif

#ifndef DECLARE_HANDLE
typedef void VOID;
typedef void* LPVOID;
typedef int BOOL;
typedef unsigned short USHORT;
typedef int INT;
typedef unsigned int UINT;
typedef long LONG;
typedef unsigned long DWORD;
typedef int INT32;
typedef __int64 INT64;
typedef float FLOAT;
typedef char CHAR;
typedef const char* LPCSTR;
typedef void* HANDLE;
typedef int (*PROC)();
struct RECT
{
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
};
#define DECLARE_HANDLE(name) typedef HANDLE name
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HGLRC);
#endif

// VC6 doesn't define wchar_t as built-in type
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;	// for glu.h
#define _WCHAR_T_DEFINED
#endif

WINGDIAPI BOOL  WINAPI wglCopyContext(HGLRC, HGLRC, UINT);
WINGDIAPI HGLRC WINAPI wglCreateContext(HDC);
WINGDIAPI HGLRC WINAPI wglCreateLayerContext(HDC, int);
WINGDIAPI BOOL  WINAPI wglDeleteContext(HGLRC);
WINGDIAPI HGLRC WINAPI wglGetCurrentContext();
WINGDIAPI HDC   WINAPI wglGetCurrentDC();
WINGDIAPI PROC  WINAPI wglGetProcAddress(LPCSTR);
WINGDIAPI BOOL  WINAPI wglMakeCurrent(HDC, HGLRC);
WINGDIAPI BOOL  WINAPI wglShareLists(HGLRC, HGLRC);
WINGDIAPI BOOL  WINAPI wglUseFontBitmapsA(HDC, DWORD, DWORD, DWORD);
WINGDIAPI BOOL  WINAPI wglUseFontBitmapsW(HDC, DWORD, DWORD, DWORD);
