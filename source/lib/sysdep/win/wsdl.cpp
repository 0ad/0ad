/*
 * emulation of a subset of SDL and GLUT for Win32
 *
 * Copyright (c) 2003 Jan Wassenberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Contact info:
 *   Jan.Wassenberg@stud.uni-karlsruhe.de
 *   http://www.stud.uni-karlsruhe.de/~urkt/
 */
 
// TODO: should use GetMessage when not active to reduce CPU load.
// where to do this?
// - force the app to check for SDL's activation messages, and call
//   sdl-wait-message?
// - do it here, just make SDL_PollEvent block until message received?

#include "precompiled.h"

#include <process.h>

#include "sdl.h"
#include "lib.h"
#include "win_internal.h"

#include "SDL_vkeys.h"

#include <assert.h>
#include <stdio.h>
#include <math.h>

#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#endif


#pragma data_seg(".LIB$WIA")
WIN_REGISTER_FUNC(wsdl_init);
#pragma data_seg(".LIB$WTA")
WIN_REGISTER_FUNC(wsdl_shutdown);
#pragma data_seg()


/* state */
static bool app_active;		/* is window active & on top?
							   if not, msg loop should yield */
	// responsibility for yielding lies with SDL apps -
	// they control the main loop.

static bool fullscreen;		/* in fullscreen mode?
							   if so, restore mode when app is deactivated */

static bool is_shutdown;

static HINSTANCE hInst = 0;

static HWND hWnd = (HWND)INVALID_HANDLE_VALUE;		/* make available to the app for ShowWindow calls, etc.? */

static DEVMODE dm;			/* current video mode */
static HDC hDC;
static HGLRC hGLRC;

static int z_depth = 24;	/* depth buffer size; set via SDL_GL_SetAttribute */

static u16 mouse_x, mouse_y;


static void gamma_swap(bool restore_org);


/*
 * shared msg handler
 * SDL and GLUT have separate pumps; messages are handled there
 */
static LRESULT CALLBACK wndproc(HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam)
{
	if(is_shutdown)
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	switch(uMsg)
	{
	case WM_PAINT:
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		return 0;

	case WM_ERASEBKGND:
		return 0;

	case WM_ACTIVATE:
		app_active = (wParam & 0xffff) != 0;

		gamma_swap(!app_active);
debug_out("active=%d\n", app_active);
		if(fullscreen)
		{
			if(app_active)
				ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
			else
				ChangeDisplaySettings(0, 0);
		}
		break;

	case WM_CLOSE:
		exit(0);

	// prevent moving, sizing, screensaver, and power-off in fullscreen mode
	case WM_SYSCOMMAND:
		switch(wParam)
		{
			case SC_MOVE:
			case SC_SIZE:
			case SC_MAXIMIZE:
			case SC_MONITORPOWER:
				if(fullscreen)
					return 1;
		}
		break;

		
	// prevent selecting menu in fullscreen mode
	case WM_NCHITTEST:
		if(fullscreen)
			return HTCLIENT;
		// else: fall through to DefWindowProc
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


int SDL_EnableUNICODE(int enable)
{
	UNUSED(enable)
	return 1;
}

// Translate Windows VK's into our SDLK's for keys that must be translated
static void init_vkmap(SDLKey (&VK_keymap)[256])
{
	int i;

	/* Map the VK keysyms */
	for ( i=0; i<sizeof(VK_keymap)/sizeof(VK_keymap[0]); ++i )
		VK_keymap[i] = SDLK_UNKNOWN;

	VK_keymap[VK_BACK] = SDLK_BACKSPACE;
	VK_keymap[VK_TAB] = SDLK_TAB;
	VK_keymap[VK_CLEAR] = SDLK_CLEAR;
	VK_keymap[VK_RETURN] = SDLK_RETURN;
	VK_keymap[VK_PAUSE] = SDLK_PAUSE;
	VK_keymap[VK_ESCAPE] = SDLK_ESCAPE;
	VK_keymap[VK_SPACE] = SDLK_SPACE;
	VK_keymap[VK_OEM_7] = SDLK_QUOTE;
	VK_keymap[VK_OEM_COMMA] = SDLK_COMMA;
	VK_keymap[VK_OEM_MINUS] = SDLK_MINUS;
	VK_keymap[VK_OEM_PERIOD] = SDLK_PERIOD;
	VK_keymap[VK_OEM_2] = SDLK_SLASH;
	for(i = 0; i < 10; i++)
		VK_keymap[VK_0+i] = (SDLKey)(SDLK_0+i);
	VK_keymap[VK_OEM_1] = SDLK_SEMICOLON;
	VK_keymap[VK_OEM_PLUS] = SDLK_EQUALS;
	VK_keymap[VK_OEM_4] = SDLK_LEFTBRACKET;
	VK_keymap[VK_OEM_5] = SDLK_BACKSLASH;
	VK_keymap[VK_OEM_6] = SDLK_RIGHTBRACKET;
	VK_keymap[VK_OEM_3] = SDLK_BACKQUOTE;
	VK_keymap[VK_OEM_8] = SDLK_BACKQUOTE;

	for(i = 0; i < 26; i++)
		VK_keymap[VK_A+i] = (SDLKey)(SDLK_a+i);

	VK_keymap[VK_DELETE] = SDLK_DELETE;

	for(i = 0; i < 10; i++)
		VK_keymap[VK_NUMPAD0+i] = (SDLKey)(SDLK_KP0+i);

	VK_keymap[VK_DECIMAL] = SDLK_KP_PERIOD;
	VK_keymap[VK_DIVIDE] = SDLK_KP_DIVIDE;
	VK_keymap[VK_MULTIPLY] = SDLK_KP_MULTIPLY;
	VK_keymap[VK_SUBTRACT] = SDLK_KP_MINUS;
	VK_keymap[VK_ADD] = SDLK_KP_PLUS;

	VK_keymap[VK_UP] = SDLK_UP;
	VK_keymap[VK_DOWN] = SDLK_DOWN;
	VK_keymap[VK_RIGHT] = SDLK_RIGHT;
	VK_keymap[VK_LEFT] = SDLK_LEFT;
	VK_keymap[VK_INSERT] = SDLK_INSERT;
	VK_keymap[VK_HOME] = SDLK_HOME;
	VK_keymap[VK_END] = SDLK_END;
	VK_keymap[VK_PRIOR] = SDLK_PAGEUP;
	VK_keymap[VK_NEXT] = SDLK_PAGEDOWN;

	for(i = 0; i < 12; i++)
		VK_keymap[VK_F1+i] = (SDLKey)(SDLK_F1+i);

	VK_keymap[VK_NUMLOCK] = SDLK_NUMLOCK;
	VK_keymap[VK_CAPITAL] = SDLK_CAPSLOCK;
	VK_keymap[VK_SCROLL] = SDLK_SCROLLOCK;
	VK_keymap[VK_RSHIFT] = SDLK_RSHIFT;
	VK_keymap[VK_LSHIFT] = SDLK_LSHIFT;
	VK_keymap[VK_SHIFT] = SDLK_LSHIFT; // XXX: Not quite
	VK_keymap[VK_RCONTROL] = SDLK_RCTRL;
	VK_keymap[VK_LCONTROL] = SDLK_LCTRL;
	VK_keymap[VK_CONTROL] = SDLK_LCTRL; // XXX: Not quite
	VK_keymap[VK_RMENU] = SDLK_RALT;
	VK_keymap[VK_LMENU] = SDLK_LALT;
	VK_keymap[VK_MENU] = SDLK_LALT; // XXX: Not quite
	VK_keymap[VK_RWIN] = SDLK_RSUPER;
	VK_keymap[VK_LWIN] = SDLK_LSUPER;

	VK_keymap[VK_HELP] = SDLK_HELP;
#ifdef VK_PRINT
	VK_keymap[VK_PRINT] = SDLK_PRINT;
#endif
	VK_keymap[VK_SNAPSHOT] = SDLK_PRINT;
	VK_keymap[VK_CANCEL] = SDLK_BREAK;
	VK_keymap[VK_APPS] = SDLK_MENU;
}

inline SDLKey vkmap(int vk)
{
	static SDLKey VK_SDLKMap[256]; /* VK_SDLKMap[vk] == SDLK */

	ONCE( init_vkmap(VK_SDLKMap); );

	assert(vk >= 0 && vk < 256);

	return VK_SDLKMap[vk];
}


// note: keysym.unicode is only returned for SDL_KEYDOWN, and is otherwise 0.
int SDL_PollEvent(SDL_Event* ev)
{
	//
	// buffer for unicode support / dead char
	//

	const int CHAR_BUF_SIZE = 8;
	static wchar_t char_buf[CHAR_BUF_SIZE];
	static uint next_char_idx=0;
	static int num_chars=0;
	static SDLKey translated_keysym=SDLK_UNKNOWN;

	// input is waiting in buffer
return_char:
	assert(num_chars <= CHAR_BUF_SIZE);
	if(num_chars > 0)
	{
		num_chars--;
		if(next_char_idx < CHAR_BUF_SIZE)
		{
			wchar_t c = char_buf[next_char_idx];
			next_char_idx++;

			ev->type = SDL_KEYDOWN;
			ev->key.keysym.sym = (SDLKey)translated_keysym;
			//ev->key.keysym.sym = (SDLKey)((c < 256)? c : 0);
			ev->key.keysym.unicode = c;
			return 1;
		}
		else
			debug_warn("SDL_PollEvent: next_char_idx invalid");
	}


	// events that trigger messages (mouse done below)
	MSG msg;
	while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
	{
		DispatchMessageW(&msg);

		int sdl_btn = -1;


		switch(msg.message)
		{

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			UINT vk = (UINT)msg.wParam;
			UINT scancode = (UINT)((msg.lParam >> 16) & 0xff);
			u8 key_states[256];
			GetKeyboardState(key_states);

			num_chars = ToUnicode(vk, scancode, key_states, char_buf, CHAR_BUF_SIZE, 0);
			next_char_idx = 0;
			if(num_chars > 0)
			{
				// Translation complete: Produce one or more Unicode chars
				char_buf[num_chars]=0;
				translated_keysym=vkmap(vk);
				//wprintf(L"ToUnicode: Translated %02x to [%s], %d chars, SDLK %02x. Extended flag %d, scancode %d\n", vk, char_buf, num_chars, translated_keysym, msg.lParam & 0x01000000, scancode);
				goto return_char;
			}
			else if (num_chars == -1)
			{
				// Dead Key: Don't produce an event for this one
				//printf("ToUnicode: Dead Key %02x [%c] [%c] SDLK %02x\n", vk, vk, char_buf[0], vkmap(vk));
				num_chars = 0;
				break;
					// leave the switch statement; get next message.
			}
			// num_chars == 0: No translation: Just produce a plain KEYDOWN event

			// TODO Mappings for left/right modifier keys
			// TODO Modifier statekeeping

			ev->type = SDL_KEYDOWN;
			ev->key.keysym.sym = vkmap(vk);
			ev->key.keysym.unicode = 0;

			//printf("ToUnicode: No translation for %02x, extended flag %d, scancode %d, SDLK %02x [%c]\n", vk, msg.lParam & 0x01000000, scancode, ev->key.keysym.sym, ev->key.keysym.sym);

			return 1;
		}

		case WM_SYSKEYUP:
		case WM_KEYUP:
			// TODO Mappings for left/right modifier keys
			// TODO Modifier statekeeping

			ev->type = SDL_KEYUP;
			ev->key.keysym.sym = vkmap((int)msg.wParam);
			ev->key.keysym.unicode = 0;
			return 1;

		case WM_MOUSEWHEEL:
			sdl_btn = (msg.wParam & BIT(31))? SDL_BUTTON_WHEELUP : SDL_BUTTON_WHEELDOWN;
			break;	// event filled in mouse code below
		}

		// mouse button
		// map Win L(up,down,double),R(),M() to L,R,M with up flag
		uint btn = msg.message-0x201;	// 0..8 if it's a valid button;
		if(btn < 9 && btn%3 != 2)		// every third msg is dblclick
			sdl_btn = SDL_BUTTON_LEFT + btn/3;	// assumes L,R,M
		if(sdl_btn != -1)
		{
			ev->type = (u8)(SDL_MOUSEBUTTONDOWN + btn%3);
			ev->button.button = (u8)sdl_btn;
			ev->button.x = (u16)(msg.lParam & 0xffff);
			ev->button.y = (u16)((msg.lParam >> 16) & 0xffff);
			return 1;
		}
	}

	// mouse motion
	//
	// don't use DirectInput, because we want to respect the user's mouse
	// sensitivity settings. Windows messages are laggy, so poll instead.
	POINT p;
	GetCursorPos(&p);
	if(mouse_x != p.x || mouse_y != p.y)
	{
		ev->type = SDL_MOUSEMOTION;
		ev->motion.x = mouse_x = (u16)p.x;
		ev->motion.y = mouse_y = (u16)p.y;
		return 1;
	}

	// app activate
	// WM_ACTIVATE is only sent to the wndproc, apparently,
	// so we have to poll here.
	static bool last_app_active = true;
		// true => suppress first event (as documented by SDL)
	if(app_active != last_app_active)
	{
		last_app_active = app_active;
	
		ev->type = SDL_ACTIVEEVENT;
		ev->active.state = SDL_APPACTIVE;
		ev->active.gain = (u8)app_active;
		return 1;
	}

	return 0;
}



int SDL_GL_SetAttribute(SDL_GLattr attr, int value)
{
	if(attr == SDL_GL_DEPTH_SIZE)
		z_depth = value;

	return 0;
}


//
// keyboard hook - used to intercept PrintScreen and system keys (e.g. Alt+Tab)
//

// note: the LowLevel hooks are global, but a DLL isn't actually required
// as stated in the docs. Windows apparently calls the handler in its original
// context. see http://www.gamedev.net/community/forums/topic.asp?topic_id=255399 .

static HHOOK hKeyboard_LL_Hook = (HHOOK)0;

LRESULT CALLBACK keyboard_ll_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if(nCode == HC_ACTION)
	{
		PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
		DWORD vk = p->vkCode;

		// replace Windows PrintScreen handler
		if(vk == VK_SNAPSHOT)
		{
			// send to wndproc
			UINT msg = (UINT)wParam;
			PostMessage(hWnd, msg, vk, 0);
				// specify hWnd to be safe.
				// if window not yet created, it's INVALID_HANDLE_VALUE.

			// don't pass it on to other handlers
			return 1;
		}
	}

	// pass it on to other hook handlers
	return CallNextHookEx(hKeyboard_LL_Hook, nCode, wParam, lParam);
}


void enable_kbd_hook(bool enable)
{
	if(enable)
	{
		assert(hKeyboard_LL_Hook == 0);
		hKeyboard_LL_Hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_ll_hook, hInst, 0);
		assert(hKeyboard_LL_Hook != 0);
	}
	else
	{
		assert(hKeyboard_LL_Hook != 0);
		UnhookWindowsHookEx(hKeyboard_LL_Hook);
		hKeyboard_LL_Hook = 0;
	}
}




// SDL redirects stdout.txt in its WinMain hook. we need to do this
// here (before main is called), instead of in SDL_Init,
// to completely emulate SDL; bonus: we don't miss output before SDL_Init.

static int wsdl_init()
{
	hInst = GetModuleHandle(0);

	// ignore BoundsChecker warnings here. subsystem is set to "Windows"
	// to avoid the OS opening a console on startup (ugly). that means
	// stdout isn't associated with a lowio handle; _close ends up
	// getting called with fd = -1. oh well, nothing we can do.
	FILE* const ret = freopen("stdout.txt", "w", stdout);
	if(!ret)
		debug_warn("stdout freopen failed");
	setvbuf(stdout, 0, _IONBF, 0);

	return 0;
}


static int wsdl_shutdown()
{
	is_shutdown = true;

	// redirected to stdout.txt in SDL_Init;
	// close to avoid BoundsChecker warning.
	fclose(stdout);

	gamma_swap(true);

	if(hDC != INVALID_HANDLE_VALUE)
	{
		ReleaseDC(hWnd, hDC);
		hDC = (HDC)INVALID_HANDLE_VALUE;
	}

	if(hWnd != INVALID_HANDLE_VALUE)
	{
		DestroyWindow(hWnd);
		hWnd = (HWND)INVALID_HANDLE_VALUE;
	}

	if(hGLRC != INVALID_HANDLE_VALUE)
	{
		wglMakeCurrent(0, 0);
		wglDeleteContext(hGLRC);
		ChangeDisplaySettings(0, 0);

		hGLRC = (HGLRC)INVALID_HANDLE_VALUE;
	}

	return 0;
}


int SDL_Init(Uint32 flags)
{
	UNUSED(flags);

	enable_kbd_hook(true);

	return 0;
}


void SDL_Quit()
{
	enable_kbd_hook(false);
}



#include "ogl.h"







/*
 * set video mode wxh:bpp if necessary.
 * w = h = bpp = 0 => no change.
 */
int SDL_SetVideoMode(int w, int h, int bpp, unsigned long flags)
{
	int ret = 0;	// assume failure
	WIN_SAVE_LAST_ERROR;	// OpenGL and GDI

	fullscreen = (flags & SDL_FULLSCREEN) != 0;

	/* get current mode settings */
	dm.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm);
	int cur_w = dm.dmPelsWidth, cur_h = dm.dmPelsHeight;

	dm.dmBitsPerPel = bpp;
	dm.dmFields = DM_BITSPERPEL;

	/*
	 * check if mode needs to be changed
	 * (could split this out, but depends on fullscreen and dm)
	 */
	if(w != 0 && h != 0 && bpp != 0)
		if(/* higher res mode needed */
		   (w > cur_w || h > cur_h) ||
		   /* fullscreen, and not exact mode */
		   (fullscreen && (w != cur_w || h != cur_h)))
	{
		dm.dmPelsWidth  = w;
		dm.dmPelsHeight = h;
		dm.dmFields |= DM_PELSWIDTH|DM_PELSHEIGHT;
	}
	// mode set at first WM_ACTIVATE

	/*
	 * window init
	 * create new window every time (instead of once at startup), 'cause
	 * pixel format isn't supposed to be changed more than once
	 */

	/* register window class */
	static WNDCLASS wc;
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = wndproc;
	wc.lpszClassName = "ogl";
	wc.hInstance = hInst;
	RegisterClass(&wc);

	hWnd = CreateWindowEx(0, "ogl", APP_NAME, WS_POPUP|WS_VISIBLE, 0, 0, w, h, 0, 0, hInst, 0);
	if(!hWnd)
		return 0;

	hDC = GetDC(hWnd);

	/* set pixel format */
	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		(BYTE)bpp,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		(BYTE)z_depth,
		0, 0,
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	// GDI pixel format functions apparently require opengl to be loaded
	// (may not have been done yet, if delay-loaded). call a gl function
	// (no-op) to make sure.
	glGetError();

	int pf = ChoosePixelFormat(hDC, &pfd);
	if(!SetPixelFormat(hDC, pf, &pfd))
		goto fail;

	hGLRC = wglCreateContext(hDC);
	if(!hGLRC)
		goto fail;

	if(!wglMakeCurrent(hDC, hGLRC))
		goto fail;

	ret = 1;

fail:
	WIN_RESTORE_LAST_ERROR;
	return ret;
}


inline void SDL_GL_SwapBuffers()
{
	SwapBuffers(hDC);
}


//////////////////////////////////////////////////////////////////////////////


static bool gamma_changed;
static u16 org_ramp[3][256];
static u16 cur_ramp[3][256];


static int calc_gamma_ramp(float gamma, u16* ramp)
{
	if(gamma <= 0.0f)
		return ERR_INVALID_PARAM;

	// identity
	// (special-case it to make sure we get exact values)
	if(gamma == 1.0f)
	{
		for(u16 i = 0; i < 256; i++)
			ramp[i] = (i << 8);
		return 0;
	}

	const double inv_gamma = 1.0 / gamma;
	for(int i = 0; i < 256; i++)
	{
		const double frac = i / 256.0;
			// make sure pow arg types are unambiguous
		ramp[i] = fp_to_u16(pow(frac, inv_gamma));
			// 8.8 fixed point
	}

	return 0;
}


static void gamma_swap(bool restore_org)
{
	if(gamma_changed)
	{
		void* ramp = (restore_org)? org_ramp : cur_ramp;
		SetDeviceGammaRamp(hDC, ramp);
	}
}


int SDL_SetGamma(float r, float g, float b)
{
	if(!gamma_changed)
		if(!GetDeviceGammaRamp(hDC, org_ramp))
			return -1;

	CHECK_ERR(calc_gamma_ramp(r, cur_ramp[0]));
	CHECK_ERR(calc_gamma_ramp(g, cur_ramp[1]));
	CHECK_ERR(calc_gamma_ramp(b, cur_ramp[2]));

	if(!SetDeviceGammaRamp(hDC, cur_ramp))
		return -1;

	gamma_changed = true;
	return 0;
}


//////////////////////////////////////////////////////////////////////////////


// implement only if the header hasn't mapped SDL_Swap* to intrinsics

#ifndef SDL_Swap16
u16 SDL_Swap16(const u16 x)
{
	return (u16)(((x & 0xff) << 8) | (x >> 8));
}
#endif

#ifndef SDL_Swap32
u32 SDL_Swap32(const u32 x)
{
	return (x << 24) |
	       (x >> 24) |
	       ((x << 8) & 0x00ff0000) |
	       ((x >> 8) & 0x0000ff00);
}
#endif

#ifndef SDL_Swap64
u64 SDL_Swap64(const u64 x)
{
	const u32 lo = (u32)(x & 0xffffffff);
	const u32 hi = (u32)(x >> 32);
	u64 ret = SDL_Swap32(lo);
	ret <<= 32;
		// careful: must shift var of type u64, not u32
	ret |= SDL_Swap32(hi);
	return ret;
}
#endif

//////////////////////////////////////////////////////////////////////////////



void SDL_WM_SetCaption(const char* title, const char* icon)
{
	SetWindowText(hWnd, title);

	UNUSED(icon);	// TODO: implement
}




#define DDRAW


#ifdef DDRAW
#include <ddraw.h>
#ifdef _MSC_VER
#pragma comment(lib, "ddraw.lib")
	// for DirectDrawCreate. don't bother with dynamic linking -
	// DirectX is present in all Windows versions since Win95.
#endif
#endif


SDL_VideoInfo* SDL_GetVideoInfo()
{
	static SDL_VideoInfo video_info;

#ifdef DDRAW

	WIN_SAVE_LAST_ERROR;	// DirectDraw

	ONCE({
		IDirectDraw* dd = 0;
		HRESULT hr = DirectDrawCreate(0, &dd, 0);
		if(SUCCEEDED(hr) && dd != 0)
		{
			static DDCAPS caps;
			caps.dwSize = sizeof(caps);
			hr = dd->GetCaps(&caps, 0);
			if(SUCCEEDED(hr))
				video_info.video_mem = caps.dwVidMemTotal;
			dd->Release();
		}
	});

	WIN_RESTORE_LAST_ERROR;

#endif

	return &video_info;
}


SDL_Surface* SDL_GetVideoSurface()
{
	return 0;
}

__declspec(naked) u32 SDL_GetTicks()
{
__asm jmp	dword ptr [GetTickCount]
}


__declspec(naked) void __stdcall SDL_Delay(Uint32 ms)
{
	UNUSED(ms);
__asm jmp	dword ptr [Sleep]
}



void* SDL_GL_GetProcAddress(const char* name)
{
	return wglGetProcAddress(name);
}


SDL_sem* SDL_CreateSemaphore(int cnt)
{
	return (SDL_sem*)CreateSemaphore(0, cnt, 0x7fffffff, 0);
}

void __stdcall SDL_DestroySemaphore(SDL_sem*)
{
__asm jmp	dword ptr [CloseHandle]
}

int SDL_SemPost(SDL_sem* sem)
{
	return ReleaseSemaphore(sem, 1, 0);
}

int SDL_SemWait(SDL_sem* sem)
{
	return WaitForSingleObject(sem, INFINITE);
}

SDL_Thread* SDL_CreateThread(int(*func)(void*), void* param)
{
	return (SDL_Thread*)_beginthread((void(*)(void*))func, 0, param);
}


int SDL_KillThread(SDL_Thread* thread)
{
	return TerminateThread(thread, 0);
}


__declspec(naked) int __stdcall SDL_WarpMouse(int, int)
{
__asm jmp	dword ptr [SetCursorPos]
}






















static bool need_redisplay;	/* display callback should be called in next main loop iteration */



/* glut callbacks */
static void (*idle)();
static void (*display)();
static void (*key)(int, int, int);
static void (*special)(int, int, int);
static void (*mouse)(int, int, int, int);

void glutIdleFunc(void (*func)())
{ idle = func; }

void glutDisplayFunc(void (*func)())
{ display = func; }

void glutKeyboardFunc(void (*func)(int, int, int))
{ key = func; }

void glutSpecialFunc(void (*func)(int, int, int))
{ special = func; }

void glutMouseFunc(void (*func)(int, int, int, int))
{ mouse = func; }









void glutInit(int* argc, char* argv[])
{
	UNUSED(argc)
	UNUSED(argv)

	SDL_Init(0);
	atexit(SDL_Quit);
}


int glutGet(int arg)
{
	if(arg == GLUT_ELAPSED_TIME)
		return GetTickCount();

	dm.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm);

	if(arg == GLUT_SCREEN_WIDTH)
		return dm.dmPelsWidth;
	if(arg == GLUT_SCREEN_HEIGHT)
		return dm.dmPelsHeight;

	return 0;
}


static int w, h, bpp, refresh;

int glutGameModeString(const char* str)
{
	/* default = "don't care", in case string doesn't specify all values */
	w = 0, h = 0, bpp = 0, refresh = 0;

	sscanf(str, "%dx%d:%d@%d", &w, &h, &bpp, &refresh);

	return 1;
}


/*
 */
int glutEnterGameMode()
{
	return SDL_SetVideoMode(w, h, bpp, SDL_OPENGL|SDL_FULLSCREEN);
}



inline void glutPostRedisplay()
{
	need_redisplay = true;
}


void glutSetCursor(int cursor)
{
	SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(cursor)));
}



/*
 * GLUT message handler
 * message also goes to the shared wndproc
 *
 * not done in wndproc to separate GLUT and SDL;
 * split out of glutMainLoop for clarity.
 */
static void glut_process_msg(MSG* msg)
{
	switch(msg->message)
	{
	case WM_PAINT:
		need_redisplay = true;
		break;

	case WM_CHAR:
		if(key)
			key((int)msg->wParam, mouse_x, mouse_y);
		break;

	case WM_KEYDOWN:
		if(special)
			special((int)msg->wParam, mouse_x, mouse_y);
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:	/* FIXME: only left/right clicks, assume GLUT_LEFT|RIGHT_BUTTON == 0, 1 */
		if(mouse)
			mouse(msg->message == WM_RBUTTONDOWN, GLUT_DOWN, (int)(msg->lParam & 0xffff), (int)(msg->lParam >> 16));
		break;

	case WM_MOUSEWHEEL:
		if(mouse)
			mouse(GLUT_MIDDLE_BUTTON, ((short)(msg->wParam >> 16) > 0)? GLUT_UP : GLUT_DOWN, 0, 0);
		break;
	}
}


void glutMainLoop()
{
	for(;;)
	{
		if(!app_active)
			WaitMessage();

		MSG msg;
		if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			glut_process_msg(&msg);

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if(idle)
			idle();

		if(need_redisplay)
		{
			need_redisplay = false;
			display();
		}
	}
}
