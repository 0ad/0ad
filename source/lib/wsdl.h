#ifndef _WIN32
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#define __WSDL_H__		// => rest of header ignored
#endif


#ifndef __WSDL_H__
#define __WSDL_H__


#include "types.h"

/* allow apps to override window name */
#ifndef APP_NAME
#define APP_NAME "ogl"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* SDL_Init flags */
#define SDL_INIT_VIDEO 0
#define SDL_INIT_AUDIO 0
#define SDL_INIT_TIMER 0
#define SDL_INIT_NOPARACHUTE 0

extern void SDL_Quit();


typedef enum
{
	SDL_GL_DEPTH_SIZE,
	SDL_GL_DOUBLEBUFFER		/* ignored - always double buffered */
}
SDL_GLattr;

extern int SDL_GL_SetAttribute(SDL_GLattr attr, int value);


/* SDL_SetVideoMode() flags */
#define SDL_OPENGL 0
#define SDL_FULLSCREEN 1

extern int SDL_SetVideoMode(int w, int h, int bpp, u32 flags);


typedef struct
{
	int w, h;
}
SDL_Surface;

extern SDL_Surface* SDL_GetVideoSurface();


typedef struct
{
	int video_mem;
}
SDL_VideoInfo;

extern SDL_VideoInfo* SDL_GetVideoInfo();


/*
 * threads / sync
 */

typedef void SDL_sem;
typedef void SDL_Thread;

extern void* SDL_GL_GetProcAddress(const char*);

extern void SDL_GL_SwapBuffers();

extern u32 SDL_GetTicks();

extern SDL_sem* SDL_CreateSemaphore(int cnt);
extern void __stdcall SDL_DestroySemaphore(SDL_sem*);
extern int SDL_SemPost(SDL_sem*);
extern int SDL_SemWait(SDL_sem* sem);

extern SDL_Thread* SDL_CreateThread(int(*)(void*), void*);
extern int SDL_KillThread(SDL_Thread*);

extern int __stdcall SDL_WarpMouse(int, int);




/* macros */

#define SDL_Init

#define SDL_GRAB_ON 0
#define SDL_WM_GrabInput(a)
#define SDL_GetError() ""




/************************************************************************************************
 * events
 ************************************************************************************************/

/* SDLKey (mapped to VK_* codes) */
typedef enum
{
	SDLK_ESCAPE	= 0x1b,
	SDLK_8 = '8',
	SDLK_9 = '9',
	SDLK_0 = '0',
	SDLK_p = 'P',
	SDLK_r = 'R',
	SDLK_s = 'S',

	SDLK_KP_ADD     = 0x6b,
	SDLK_KP_MINUS   = 0x6d,
	SDLK_LEFT       = 0x25,
	SDLK_UP         = 0x26,
	SDLK_RIGHT      = 0x27,
	SDLK_DOWN       = 0x28,

	SDLK_KP0        = 0x60,
	SDLK_KP1        = 0x61,
	SDLK_KP2        = 0x62,
	SDLK_KP3        = 0x63,
	SDLK_KP4        = 0x64,
	SDLK_KP5        = 0x65,
	SDLK_KP6        = 0x66,
	SDLK_KP7        = 0x67,
	SDLK_KP8        = 0x68,
	SDLK_KP9        = 0x69,

__SDLK	// hack to allow trailing comma
}
SDLKey;

typedef struct
{
	SDLKey sym;
}
SDL_keysym;

typedef struct
{
	SDL_keysym keysym;
}
SDL_KeyboardEvent;

typedef struct
{
	u16 x, y;
}
SDL_MouseMotionEvent;

/* SDL_MouseButtonEvent.button */
enum
{
	// do not change order
	SDL_BUTTON_LEFT,
	SDL_BUTTON_RIGHT,
	SDL_BUTTON_MIDDLE,

	SDL_BUTTON_WHEELUP,
	SDL_BUTTON_WHEELDOWN
};

typedef struct
{
	u8 button;
	u8 state;
	u16 x, y;
}
SDL_MouseButtonEvent;

typedef struct
{
	u8 gain;
	u8 state;
}
SDL_ActiveEvent;

/* SDL_Event.type */
enum
{
	SDL_KEYDOWN,
	SDL_KEYUP,
	SDL_MOUSEMOTION,
	SDL_MOUSEBUTTONDOWN,
	SDL_MOUSEBUTTONUP,
	SDL_ACTIVE
};

typedef struct
{
	u8 type;
	union
	{
		SDL_KeyboardEvent key;
		SDL_MouseMotionEvent motion;
		SDL_MouseButtonEvent button;
		SDL_ActiveEvent active;
	};
}
SDL_Event;

extern int SDL_PollEvent(SDL_Event* ev);















/* glutInitDisplayMode */
#define GLUT_RGB			0
#define GLUT_DOUBLE			0
#define GLUT_DEPTH			0

/* mouse buttons */
enum
{
	GLUT_LEFT_BUTTON,
	GLUT_RIGHT_BUTTON,
	GLUT_MIDDLE_BUTTON		/* also wheel, if avail */
};

/* mouse button state */
enum
{
	GLUT_DOWN,
	GLUT_UP
};

/* keys */
enum
{
	GLUT_KEY_LEFT  = 0x25,			/* VK_* */
	GLUT_KEY_RIGHT = 0x27,
	GLUT_KEY_UP    = 0x26,
	GLUT_KEY_DOWN  = 0x28
};


/* glutSetCursor */
#define GLUT_CURSOR_INHERIT	32512	/* IDC_* */
#define GLUT_CURSOR_WAIT	32514
#define GLUT_CURSOR_DESTROY	32648
#define GLUT_CURSOR_NONE	0

/* glutGet */
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


#define glutInitDisplayMode(a)	/* pixel format is hardwired */

extern int glutGameModeString(const char* str);



extern void glutInit(int* argc, char* argv[]);
extern int glutGet(int arg);
extern int glutEnterGameMode();
extern void glutMainLoop();

extern void glutPostRedisplay();

extern void glutSetCursor(int);



#define glutSwapBuffers SDL_GL_SwapBuffers



#ifdef __cplusplus
}
#endif

#endif	// #ifndef __WSDL_H__
