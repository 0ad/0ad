// emulation of a subset of SDL and GLUT for Win32
//
// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef WSDL_H__
#define WSDL_H__

#include "lib/types.h"
#include "SDL_keysym.h"

// allow apps to override window name
#ifndef APP_NAME
#define APP_NAME "ogl"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef u8  Uint8;
typedef u16 Uint16;
typedef u32 Uint32;


// SDL_Init flags
#define SDL_INIT_VIDEO 0
#define SDL_INIT_AUDIO 0
#define SDL_INIT_TIMER 0
#define SDL_INIT_NOPARACHUTE 0

extern int SDL_Init(Uint32 flags);

extern void SDL_Quit(void);


typedef enum
{
	SDL_GL_DEPTH_SIZE,
	SDL_GL_DOUBLEBUFFER		// ignored - always double buffered
}
SDL_GLattr;

extern int SDL_GL_SetAttribute(SDL_GLattr attr, int value);


// SDL_SetVideoMode() flags
#define SDL_OPENGL 0
#define SDL_FULLSCREEN 1

extern int SDL_SetVideoMode(int w, int h, int bpp, unsigned long flags);


typedef struct
{
	int w, h;
}
SDL_Surface;

extern SDL_Surface* SDL_GetVideoSurface(void);


typedef struct
{
	int video_mem;
}
SDL_VideoInfo;

extern SDL_VideoInfo* SDL_GetVideoInfo(void);


//
// threads / sync
//

typedef void SDL_sem;
typedef void SDL_Thread;

extern void* SDL_GL_GetProcAddress(const char*);

extern void SDL_GL_SwapBuffers(void);

extern u32 SDL_GetTicks(void);
extern void SDL_Delay(u32 ms);

extern SDL_sem* SDL_CreateSemaphore(int cnt);
extern void SDL_DestroySemaphore(SDL_sem*);
extern int SDL_SemPost(SDL_sem*);
extern int SDL_SemWait(SDL_sem* sem);

extern SDL_Thread* SDL_CreateThread(int(*)(void*), void*);
extern int SDL_KillThread(SDL_Thread*);

extern int SDL_WarpMouse(int, int);

enum ShowCursorToggle
{
	SDL_DISABLE = 0,
	SDL_ENABLE  = 1,
	SDL_QUERY   = 2
};
extern int SDL_ShowCursor(int toggle);


extern int SDL_SetGamma(float r, float g, float b);

// macros

#define SDL_GRAB_ON 0
#define SDL_WM_GrabInput(a)
#define SDL_GetError() ""


//////////////////////////////////////////////////////////////////////////////


#ifdef linux
# include <asm/byteorder.h>
# ifdef __arch__swab16
#  define SDL_Swap16  __arch__swab16
# endif
# ifdef __arch__swab32
#  define SDL_Swap32  __arch__swab32
# endif
#endif

// Debug-mode ICC doesn't like the intrinsics, so only use them
// for MSVC and non-debug ICC.
#if MSC_VERSION && !( defined(__INTEL_COMPILER) && !defined(NDEBUG) )
extern unsigned short _byteswap_ushort(unsigned short);
extern unsigned long _byteswap_ulong(unsigned long);
extern unsigned __int64 _byteswap_uint64(unsigned __int64);
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)
# define SDL_Swap16 _byteswap_ushort
# define SDL_Swap32 _byteswap_ulong
# define SDL_Swap64 _byteswap_uint64
#endif

#ifndef SDL_Swap16
extern u16 SDL_Swap16(u16);
#endif

#ifndef SDL_Swap32
extern u32 SDL_Swap32(u32);
#endif

#ifndef SDL_Swap64
extern u64 SDL_Swap64(u64);
#endif

#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321

#define SDL_BYTE_ORDER SDL_LIL_ENDIAN

//////////////////////////////////////////////////////////////////////////////
//
// events
//
//////////////////////////////////////////////////////////////////////////////

typedef struct
{
	Uint8 type;
	SDLKey sym;
	u16 unicode;
}
SDL_keysym;

typedef struct
{
	Uint8 type;
	SDL_keysym keysym;
}
SDL_KeyboardEvent;

typedef struct
{
	Uint8 type;
	u16 x, y;
}
SDL_MouseMotionEvent;

typedef struct
{
	Uint8 type;
}
SDL_QuitEvent;

enum SDL_MouseButtonEvent_button
{
	// to remain compatible with regular SDL, these values must not change!
	SDL_BUTTON_LEFT      = 1,
	SDL_BUTTON_MIDDLE    = 2,
	SDL_BUTTON_RIGHT     = 3,
	SDL_BUTTON_WHEELUP   = 4,
	SDL_BUTTON_WHEELDOWN = 5
};

enum SDL_MouseButtonEvent_state
{
	SDL_RELEASED = 0,
	SDL_PRESSED  = 1
};

typedef struct
{
	Uint8 type;
	u8 button;
	u8 state;
	u16 x, y;
}
SDL_MouseButtonEvent;

enum SDL_ActiveEvent_state
{
	SDL_APPACTIVE     = 1,
	SDL_APPMOUSEFOCUS = 2,
	SDL_APPINPUTFOCUS = 4
};

typedef struct
{
	Uint8 type;
	u8 gain;
	u8 state;
}
SDL_ActiveEvent;

typedef struct
{
	Uint8 type;
	int code;
	void* data1;
}
SDL_UserEvent;

enum SDL_Event_type
{
	SDL_KEYDOWN,
	SDL_KEYUP,
	SDL_MOUSEMOTION,
	SDL_MOUSEBUTTONDOWN,
	SDL_MOUSEBUTTONUP,
	SDL_ACTIVEEVENT,
	SDL_QUIT,
	SDL_USEREVENT
};

typedef union
{
	Uint8 type;
	SDL_KeyboardEvent key;
	SDL_MouseMotionEvent motion;
	SDL_MouseButtonEvent button;
	SDL_ActiveEvent active;
	SDL_UserEvent user;
}
SDL_Event;


extern int SDL_EnableUNICODE(int enable);
extern int SDL_WaitEvent(SDL_Event*);
extern int SDL_PollEvent(SDL_Event* ev);
extern int SDL_PushEvent(SDL_Event* ev);

extern void SDL_WM_SetCaption(const char *title, const char *icon);













// glutInitDisplayMode
#define GLUT_RGB			0
#define GLUT_DOUBLE			0
#define GLUT_DEPTH			0

// mouse buttons
enum
{
	GLUT_LEFT_BUTTON,
	GLUT_RIGHT_BUTTON,
	GLUT_MIDDLE_BUTTON		// also wheel, if avail
};

// mouse button state
enum
{
	GLUT_DOWN,
	GLUT_UP
};

// keys
enum
{
	GLUT_KEY_LEFT  = 0x25,			// VK_*
	GLUT_KEY_RIGHT = 0x27,
	GLUT_KEY_UP    = 0x26,
	GLUT_KEY_DOWN  = 0x28
};


// glutSetCursor
#define GLUT_CURSOR_INHERIT	32512	// IDC_*
#define GLUT_CURSOR_WAIT	32514
#define GLUT_CURSOR_DESTROY	32648
#define GLUT_CURSOR_NONE	0

// glutGet
enum
{
GLUT_ELAPSED_TIME,
GLUT_SCREEN_WIDTH,
GLUT_SCREEN_HEIGHT,

GLUT_GAME_MODE_WIDTH,
GLUT_GAME_MODE_HEIGHT,
GLUT_GAME_MODE_PIXEL_DEPTH,
GLUT_GAME_MODE_REFRESH_RATE
};



extern void glutIdleFunc(void(*)());
extern void glutDisplayFunc(void(*)());
extern void glutKeyboardFunc(void(*)(int, int, int));
extern void glutSpecialFunc(void(*)(int, int, int));
extern void glutMouseFunc(void(*)(int, int, int, int));


#define glutInitDisplayMode(a)	// pixel format is hardwired

extern int glutGameModeString(const char* str);



extern void glutInit(int* argc, char* argv[]);
extern int glutGet(int arg);
extern int glutEnterGameMode(void);
extern void glutMainLoop(void);

extern void glutPostRedisplay(void);

extern void glutSetCursor(int);



#define glutSwapBuffers SDL_GL_SwapBuffers



#ifdef __cplusplus
}
#endif

#endif	// #ifndef WSDL_H__
