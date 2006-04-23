/**
 * =========================================================================
 * File        : wsdl.h
 * Project     : 0 A.D.
 * Description : emulate SDL on Windows.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2004 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef WSDL_H__
#define WSDL_H__

#include "lib/types.h"
#include "SDL_keysym.h"

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


//
// video
//

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

extern void* SDL_GL_GetProcAddress(const char*);

extern void SDL_GL_SwapBuffers(void);


//
// threads / sync
//

typedef void SDL_sem;
typedef void SDL_Thread;

extern u32 SDL_GetTicks(void);
extern void SDL_Delay(u32 ms);

extern SDL_sem* SDL_CreateSemaphore(int cnt);
extern void SDL_DestroySemaphore(SDL_sem*);
extern int SDL_SemPost(SDL_sem*);
extern int SDL_SemWait(SDL_sem* sem);

extern SDL_Thread* SDL_CreateThread(int (*)(void*), void*);
extern int SDL_KillThread(SDL_Thread*);


extern void SDL_WarpMouse(int, int);

enum ShowCursorToggle
{
	SDL_DISABLE = 0,
	SDL_ENABLE  = 1,
	SDL_QUERY   = 2
};
extern int SDL_ShowCursor(int toggle);


extern int SDL_SetGamma(float r, float g, float b);


//
// byte swapping
//


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

#define SDL_BUTTON(b)		(SDL_PRESSED << (b-1))
#define SDL_BUTTON_LMASK	SDL_BUTTON(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK	SDL_BUTTON(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK	SDL_BUTTON(SDL_BUTTON_RIGHT)

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


//
// misc
//

#define SDL_GRAB_ON 0
#define SDL_WM_GrabInput(a)

#define SDL_GetError() ""

// from real SDL, but they're ignored anyway
#define SDL_DEFAULT_REPEAT_DELAY	500
#define SDL_DEFAULT_REPEAT_INTERVAL	30
#define SDL_EnableKeyRepeat(delay, interval)


extern void SDL_WM_SetCaption(const char *title, const char *icon);

extern Uint8* SDL_GetKeyState(int* num_keys);
extern Uint8 SDL_GetMouseState(int* x, int* y);

extern Uint8 SDL_GetAppState();


#ifdef __cplusplus
}
#endif

#endif	// #ifndef WSDL_H__
