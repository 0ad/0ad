// RAGE! Win32 OpenGL headers are full of crap we have to emulate
// (must not include windows.h)

#ifndef _WIN32
#error "do not include if not compiling for Windows"
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
typedef int(*PROC)(void);
#define DECLARE_HANDLE(name) typedef HANDLE name
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HGLRC);
#endif

typedef unsigned short wchar_t;	// for glu.h

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