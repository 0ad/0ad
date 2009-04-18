/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : wgl.cpp
 * Project     : 0 A.D.
 * Description : Windows definitions required for GL/gl.h
 * =========================================================================
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
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef int INT32;
typedef __int64 INT64;
typedef float FLOAT;
typedef const char* LPCSTR;
typedef void* HANDLE;
typedef int (*PROC)(void);
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
WINGDIAPI HGLRC WINAPI wglGetCurrentContext(void);
WINGDIAPI HDC   WINAPI wglGetCurrentDC(void);
WINGDIAPI PROC  WINAPI wglGetProcAddress(LPCSTR);
WINGDIAPI BOOL  WINAPI wglMakeCurrent(HDC, HGLRC);
WINGDIAPI BOOL  WINAPI wglShareLists(HGLRC, HGLRC);
WINGDIAPI BOOL  WINAPI wglUseFontBitmapsA(HDC, DWORD, DWORD, DWORD);
WINGDIAPI BOOL  WINAPI wglUseFontBitmapsW(HDC, DWORD, DWORD, DWORD);
