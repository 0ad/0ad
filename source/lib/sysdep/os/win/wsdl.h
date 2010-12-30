/* Copyright (c) 2010 Wildfire Games
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
 * emulate SDL on Windows.
 */

#ifndef INCLUDED_WSDL
#define INCLUDED_WSDL

#include "lib/lib_api.h"
#include "lib/byte_order.h"
#include "SDL/SDL_keysym.h"

typedef u8  Uint8;
typedef u16 Uint16;
typedef i16 Sint16;
typedef u32 Uint32;


// SDL_Init flags
#define SDL_INIT_VIDEO 0
#define SDL_INIT_AUDIO 0
#define SDL_INIT_TIMER 0
#define SDL_INIT_JOYSTICK 0
#define SDL_INIT_NOPARACHUTE 0

LIB_API int SDL_Init(Uint32 flags);

LIB_API void SDL_Quit();


//
// video
//

typedef enum
{
	SDL_GL_DEPTH_SIZE,
	SDL_GL_DOUBLEBUFFER,	// ignored - always double buffered
	SDL_GL_SWAP_CONTROL
}
SDL_GLattr;

LIB_API int SDL_GL_SetAttribute(SDL_GLattr attr, int value);

// SDL_SetVideoMode() flags
#define SDL_OPENGL 0
#define SDL_FULLSCREEN 1
#define SDL_RESIZABLE 2

typedef struct
{
	int w, h;
}
SDL_Surface;

LIB_API SDL_Surface* SDL_SetVideoMode(int w, int h, int bpp, Uint32 flags);

LIB_API SDL_Surface* SDL_GetVideoSurface();

typedef struct
{
	int video_mem;
}
SDL_VideoInfo;

LIB_API SDL_VideoInfo* SDL_GetVideoInfo();

LIB_API void* SDL_GL_GetProcAddress(const char*);

LIB_API void SDL_GL_SwapBuffers();

LIB_API int SDL_SetGamma(float r, float g, float b);


//
// semaphores
//

typedef void SDL_sem;
LIB_API SDL_sem* SDL_CreateSemaphore(int cnt);
LIB_API void SDL_DestroySemaphore(SDL_sem*);
LIB_API int SDL_SemPost(SDL_sem*);
LIB_API int SDL_SemWait(SDL_sem* sem);


//
// time
//

LIB_API u32 SDL_GetTicks();
LIB_API void SDL_Delay(u32 ms);


//
// mouse
//

LIB_API void SDL_WarpMouse(int, int);

enum ShowCursorToggle
{
	SDL_DISABLE = 0,
	SDL_ENABLE  = 1,
	SDL_QUERY   = 2
};
LIB_API int SDL_ShowCursor(int toggle);

LIB_API Uint8 SDL_GetMouseState(int* x, int* y);


//
// keyboard
//

// from real SDL, but they're ignored anyway
#define SDL_DEFAULT_REPEAT_DELAY	500
#define SDL_DEFAULT_REPEAT_INTERVAL	30
#define SDL_EnableKeyRepeat(delay, interval)

LIB_API Uint8* SDL_GetKeyState(int* num_keys);


//
// joystick
//

typedef void* SDL_Joystick;
int SDL_NumJoysticks();
int SDL_JoystickEventState(int state);
SDL_Joystick* SDL_JoystickOpen(int device_index);
int SDL_JoystickNumAxes(SDL_Joystick* joystick);
Sint16 SDL_JoystickGetAxis(SDL_Joystick* joystick, int axis);


//
// byte swapping
//

#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321

#define SDL_BYTEORDER SDL_LIL_ENDIAN

#define SDL_Swap16 swap16
#define SDL_Swap32 swap32
#define SDL_Swap64 swap64


//-----------------------------------------------------------------------------
// events
//-----------------------------------------------------------------------------

typedef struct
{
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

typedef struct
{
	Uint8 type;
}
SDL_ExposeEvent;

enum SDL_MouseButtonEvent_button
{
	// to remain compatible with regular SDL, these values must not change!
	SDL_BUTTON_LEFT      = 1,
	SDL_BUTTON_MIDDLE    = 2,
	SDL_BUTTON_RIGHT     = 3,
	SDL_BUTTON_WHEELUP   = 4,
	SDL_BUTTON_WHEELDOWN = 5,
	SDL_BUTTON_X1 = 6,
	SDL_BUTTON_X2 = 7
};

#define SDL_BUTTON(b)		(1u << (b-1))
#define SDL_BUTTON_LMASK	SDL_BUTTON(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK	SDL_BUTTON(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK	SDL_BUTTON(SDL_BUTTON_RIGHT)
#define SDL_BUTTON_X1MASK	SDL_BUTTON(SDL_BUTTON_X1)
#define SDL_BUTTON_X2MASK	SDL_BUTTON(SDL_BUTTON_X2)

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
	int w;
	int h;
}
SDL_ResizeEvent;

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
	SDL_VIDEOEXPOSE,
	SDL_VIDEORESIZE,
	SDL_USEREVENT
};

typedef union
{
	Uint8 type;
	SDL_ActiveEvent active;
	SDL_KeyboardEvent key;
	SDL_MouseMotionEvent motion;
	SDL_MouseButtonEvent button;
	SDL_ResizeEvent resize;
	SDL_ExposeEvent expose;
	SDL_QuitEvent quit;
	SDL_UserEvent user;
}
SDL_Event;

LIB_API int SDL_EnableUNICODE(int enable);
LIB_API int SDL_WaitEvent(SDL_Event*);
LIB_API int SDL_PollEvent(SDL_Event* ev);
LIB_API int SDL_PushEvent(SDL_Event* ev);


//
// misc
//

enum SDL_GrabMode
{
	SDL_GRAB_QUERY,
	SDL_GRAB_OFF,
	SDL_GRAB_ON
};
LIB_API SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode);

#define SDL_GetError() ""

LIB_API void SDL_WM_SetCaption(const char *title, const char *icon);

LIB_API Uint8 SDL_GetAppState();

// Pretend that we always implement the latest version of SDL
#define SDL_VERSION_ATLEAST(X, Y, Z) 1

#endif	// #ifndef INCLUDED_WSDL
