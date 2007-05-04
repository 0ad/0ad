/**
 * =========================================================================
 * File        : wsdl.cpp
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

#include "precompiled.h"
#include "lib/external_libraries/sdl.h"

#include <stdio.h>
#include <math.h>
#include <queue>
#include <algorithm>

#include "win_internal.h"
#include <ddraw.h>
#include <process.h>	// _beginthreadex
#include <WindowsX.h>	// message crackers

#include "lib/posix/posix_pthread.h"
#include "lib/lib.h"
#include "lib/ogl.h"		// needed to pull in the delay-loaded opengl32.dll
#include "winit.h"
#include "wutil.h"


// for easy removal of DirectDraw dependency (used to query total video mem)
#define DDRAW

#if MSC_VERSION
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// don't bother with dynamic linking -
// DirectX is present in all Windows versions since Win95.
#ifdef DDRAW
# pragma comment(lib, "ddraw.lib")
#endif

#endif



#pragma SECTION_PRE_MAIN(K)
WIN_REGISTER_FUNC(wsdl_init);
#pragma FORCE_INCLUDE(wsdl_init)
#pragma SECTION_POST_ATEXIT(D)
WIN_REGISTER_FUNC(wsdl_shutdown);
#pragma FORCE_INCLUDE(wsdl_shutdown)
#pragma SECTION_RESTORE


// in fullscreen mode, i.e. not windowed.
// video mode will be restored when app is deactivated.
static bool fullscreen;

// the app is shutting down.
// if set, ignore further Windows messages for clean shutdown.
static bool is_shutdown;

// app instance.
// returned by GetModuleHandle and used in kbd hook and window creation. 
static HINSTANCE hInst = 0;

HWND hWnd = (HWND)INVALID_HANDLE_VALUE;
static HDC hDC = (HDC)INVALID_HANDLE_VALUE;	// needed by gamma code


//----------------------------------------------------------------------------
// gamma

static bool gamma_changed;
static u16 org_ramp[3][256];
static u16 cur_ramp[3][256];


// ramp: 8.8 fixed point
static LibError calc_gamma_ramp(float gamma, u16* ramp)
{
	// assume identity if invalid
	if(gamma <= 0.0f)
		gamma = 1.0f;

	// identity: special-case to make sure we get exact values
	if(gamma == 1.0f)
	{
		for(u16 i = 0; i < 256; i++)
			ramp[i] = (i << 8);
		return INFO::OK;
	}

	const double inv_gamma = 1.0 / gamma;
	for(int i = 0; i < 256; i++)
	{
		const double frac = i / 256.0;
		// don't add 1/256 - this isn't time-critical and
		// accuracy is more important.
		// need a temp variable to disambiguate pow() argument type.
		ramp[i] = fp_to_u16(pow(frac, inv_gamma));
	}

	return INFO::OK;
}


enum GammaAction
{
	GAMMA_LATCH_NEW_RAMP,
	GAMMA_RESTORE_ORIGINAL
};

static void gamma_swap(GammaAction action)
{
	if(gamma_changed)
	{
		void* ramp = (action == GAMMA_LATCH_NEW_RAMP)? cur_ramp : org_ramp;
		SetDeviceGammaRamp(hDC, ramp);
	}
}


// note: any component gamma = 0 is assumed to be identity.
int SDL_SetGamma(float r, float g, float b)
{
	// if we haven't successfully changed gamma yet,
	// get current ramp so we can later restore it.
	if(!gamma_changed)
		if(!GetDeviceGammaRamp(hDC, org_ramp))
			return -1;

	LibError err1 = calc_gamma_ramp(r, cur_ramp[0]);
	LibError err2 = calc_gamma_ramp(g, cur_ramp[1]);
	LibError err3 = calc_gamma_ramp(b, cur_ramp[2]);
	if(err1 != INFO::OK || err2 != INFO::OK || err3 != INFO::OK)
		return -1;

	if(!SetDeviceGammaRamp(hDC, cur_ramp))
		return -1;

	gamma_changed = true;
	return 0;
}


//----------------------------------------------------------------------------
// video
//----------------------------------------------------------------------------

static DEVMODE dm;			// current video mode
static HGLRC hGLRC = (HGLRC)INVALID_HANDLE_VALUE;

static int depth_bits = 24;	// depth buffer size; set via SDL_GL_SetAttribute

// check if resolution needs to be changed
static bool video_need_change(int w, int h, int cur_w, int cur_h, bool fullscreen)
{
	// invalid: keep current settings
	if(w <= 0 || h <= 0)
		return false;

	// higher resolution mode needed
	if(w > cur_w || h > cur_h)
		return true;

	// fullscreen requested and not exact same mode set
	if(fullscreen && (w != cur_w || h != cur_h))
		return true;

	return false;
}


static inline void video_enter_game_mode()
{
	ShowWindow(hWnd, SW_RESTORE);
	ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
}

static inline void video_leave_game_mode()
{
	ChangeDisplaySettings(0, 0);
	ShowWindow(hWnd, SW_MINIMIZE);
}


int SDL_GL_SetAttribute(SDL_GLattr attr, int value)
{
	if(attr == SDL_GL_DEPTH_SIZE)
		depth_bits = value;

	return 0;
}


static LRESULT CALLBACK wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// set video mode wxh:bpp if necessary.
// w = h = bpp = 0 => no change.
int SDL_SetVideoMode(int w, int h, int bpp, unsigned long flags)
{
	int ret = 0;	// assume failure
	WIN_SAVE_LAST_ERROR;	// OpenGL and GDI

	fullscreen = (flags & SDL_FULLSCREEN) != 0;

	// get current mode settings
	memset(&dm, 0, sizeof(dm));
	dm.dmSize = sizeof(dm);
	EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm);
	int cur_w = (int)dm.dmPelsWidth, cur_h = (int)dm.dmPelsHeight;

	// independent of resolution; app must always get bpp it wants
	dm.dmBitsPerPel = bpp;
	dm.dmFields = DM_BITSPERPEL;

	if(video_need_change(w,h, cur_w,cur_h, fullscreen))
	{
		dm.dmPelsWidth  = (DWORD)w;
		dm.dmPelsHeight = (DWORD)h;
		dm.dmFields |= DM_PELSWIDTH|DM_PELSHEIGHT;
	}


	// the (possibly changed) mode will be (re)set at next WM_ACTIVATE

	//
	// window init
	// create new window every time (instead of once at startup), because
	// pixel format isn't supposed to be changed more than once
	//

	// register window class
	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = wndproc;
	wc.lpszClassName = "WSDL";
	wc.hInstance = hInst;
	ATOM class_atom = RegisterClass(&wc);
	if(!class_atom)
	{
		debug_warn("SDL_SetVideoMode: RegisterClass failed");
		return 0;
	}

	DWORD windowStyle = fullscreen ? (WS_POPUP|WS_VISIBLE) : WS_VISIBLE | WS_CAPTION|WS_POPUPWINDOW|WS_MINIMIZEBOX;

	// Calculate the size of the outer window, so that the client area has
	// the desired dimensions.
	RECT r;
	r.left = r.top = 0;
	r.right = w; r.bottom = h;
	if (AdjustWindowRectEx(&r, windowStyle, FALSE, 0))
	{
		w = r.right - r.left;
		h = r.bottom - r.top;
	}

	// note: you can override the hardcoded window name via SDL_WM_SetCaption.
	hWnd = CreateWindowEx(0, (LPCSTR)class_atom, "wsdl", windowStyle, 0, 0, w, h, 0, 0, hInst, 0);
	if(!hWnd)
		return 0;

	hDC = GetDC(hWnd);


	//
	// pixel format
	//

	const DWORD dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
	BYTE cColourBits = (BYTE)bpp;
	BYTE cAlphaBits = 0;
	if(bpp == 32)
	{
		cColourBits = 24;
		cAlphaBits = 8;
	}
	const BYTE cAccumBits   = 0;
	const BYTE cDepthBits   = (BYTE)depth_bits;
	const BYTE cStencilBits = 0;
	const BYTE cAuxBuffers  = 0;

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
			1,								// version
			dwFlags,
			PFD_TYPE_RGBA,
			cColourBits, 0, 0, 0, 0, 0, 0,	// c*Bits, c*Shift are unused
			cAlphaBits, 0,					// cAlphaShift is unused
			cAccumBits,	0, 0, 0, 0,			// cAccum*Bits are unused
			cDepthBits,
			cStencilBits,
			cAuxBuffers,
			PFD_MAIN_PLANE,
			0, 0, 0, 0						// bReserved, dw*Mask are unused
	};

	// GDI pixel format functions apparently require opengl to be loaded
	// (may not have been done yet, if delay-loaded). call a gl function
	// (no-op) to make sure.
	(void)glGetError();

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


static void video_shutdown()
{
	if(fullscreen)
		if(ChangeDisplaySettings(0, 0) != DISP_CHANGE_SUCCESSFUL)
			debug_warn("ChangeDisplaySettings failed");

	if(hGLRC != INVALID_HANDLE_VALUE)
	{
		WARN_IF_FALSE(wglMakeCurrent(0, 0));
		WARN_IF_FALSE(wglDeleteContext(hGLRC));
		hGLRC = (HGLRC)INVALID_HANDLE_VALUE;
	}

	if(hDC != INVALID_HANDLE_VALUE)
	{
		// return value is whether the DC was actually freed.
		// this seems to sometimes be 0, so don't warn.
		(void)ReleaseDC(hWnd, hDC);
		hDC = (HDC)INVALID_HANDLE_VALUE;
	}

	if(hWnd != INVALID_HANDLE_VALUE)
	{
		// this also seems to fail spuriously with GetLastError == 0,
		// so don't complain.
		(void)DestroyWindow(hWnd);
		hWnd = (HWND)INVALID_HANDLE_VALUE;
	}
}


void SDL_GL_SwapBuffers()
{
	SwapBuffers(hDC);
}


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
			DDCAPS caps;
			memset(&caps, 0, sizeof(caps));
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


// For very [very] basic memory-usage information.
// Should be replaced by a decent memory profiler.
//
// copied from SDL_GetVideoInfo but cannot be implemented in terms of it:
// SDL_VideoInfo doesn't provide for returning "remaining video memory".
int GetVRAMInfo(int& remaining, int& total)
{
	int ok = 0;
#ifdef DDRAW

	WIN_SAVE_LAST_ERROR;	// DirectDraw

	IDirectDraw* dd = 0;
	HRESULT hr = DirectDrawCreate(0, &dd, 0);
	if(SUCCEEDED(hr) && dd != 0)
	{
		static DDCAPS caps;
		caps.dwSize = sizeof(caps);
		hr = dd->GetCaps(&caps, 0);
		if(SUCCEEDED(hr))
		{
			ok = 1;
			remaining = caps.dwVidMemFree;
			total = caps.dwVidMemTotal;
		}
		dd->Release();
	}

	WIN_RESTORE_LAST_ERROR;

#endif

	return ok;
}


SDL_Surface* SDL_GetVideoSurface()
{
	return 0;
}


//----------------------------------------------------------------------------
// event queue

typedef std::queue<SDL_Event> Queue;
static Queue queue;

static void queue_event(const SDL_Event& ev)
{
	queue.push(ev);
}

static bool dequeue_event(SDL_Event* ev)
{
	if(queue.empty())
		return false;
	*ev = queue.front();
	queue.pop();
	return true;
}



static void queue_quit_event()
{
	SDL_Event ev;
	ev.type = SDL_QUIT;
	queue_event(ev);
}





//----------------------------------------------------------------------------
// app activation

enum SdlActivationType { LOSE = 0, GAIN = 1 };

static inline void queue_active_event(SdlActivationType type, uint changed_app_state)
{
	// SDL says this event is not generated when the window is created,
	// but skipping the first event may confuse things.

	SDL_Event ev;
	ev.type = SDL_ACTIVEEVENT;
	ev.active.state = (u8)changed_app_state;
	ev.active.gain  = (type == GAIN)? 1 : 0;
	queue_event(ev);
}


// SDL_APP* bitflags indicating whether we are active.
// note: responsibility for yielding lies with SDL apps -
// they control the main loop.
static uint app_state;

static void active_change_state(SdlActivationType type, uint changed_app_state)
{
	uint old_app_state = app_state;

	if(type == GAIN)
		app_state |= changed_app_state;
	else
		app_state &= ~changed_app_state;

	// generate an event - but only if the given state flags actually changed.
	if((old_app_state & changed_app_state) != (app_state & changed_app_state))
		queue_active_event(type, changed_app_state);
}

static void reset_all_keys();

static LRESULT OnActivate(HWND hWnd, UINT state, HWND UNUSED(hWndActDeact), BOOL fMinimized)
{
	SdlActivationType type;
	uint changed_app_state;

	// went active and not minimized
	if(state != WA_INACTIVE && !fMinimized)
	{
		type = GAIN;
		changed_app_state = SDL_APPINPUTFOCUS|SDL_APPACTIVE;


		// grab keyboard focus (we previously had DefWindowProc do this).
		SetFocus(hWnd);

		gamma_swap(GAMMA_LATCH_NEW_RAMP);
		if(fullscreen)
			video_enter_game_mode();
	}
	// deactivated (Alt+Tab out) or minimized
	else
	{
		type = LOSE;
		changed_app_state = SDL_APPINPUTFOCUS;
		if(fMinimized)
			changed_app_state |= SDL_APPACTIVE;


		reset_all_keys();

		gamma_swap(GAMMA_RESTORE_ORIGINAL);
		if(fullscreen)
			video_leave_game_mode();
	}

	active_change_state(type, changed_app_state);
	return 0;
}


Uint8 SDL_GetAppState()
{
	return app_state;
}

//----------------------------------------------------------------------------
// keyboard

// note: keysym.unicode is only returned for SDL_KEYDOWN, and is otherwise 0.
static void queue_key_event(uint type, uint sdlk, WCHAR unicode_char)
{
	SDL_Event ev;
	ev.type = type;
	ev.key.keysym.sym     = (SDLKey)sdlk;
	ev.key.keysym.unicode = (u16)unicode_char;
	queue_event(ev);
}


static Uint8 keys[SDLK_LAST];

// winuser.h promises VK_0 and VK_A etc. match ASCII value.
#define VK_0 '0'
#define VK_A 'A'

static void init_vkmap(SDLKey (&VK_keymap)[256])
{
	int i;

	// Map the VK keysyms
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
	static SDLKey VK_SDLKMap[256]; // VK_SDLKMap[vk] == SDLK
	ONCE( init_vkmap(VK_SDLKMap); );

	if(!(0 <= vk && vk < 256))
	{
		debug_warn("vkmap: invalid vk");
		return SDLK_UNKNOWN;
	}
	return VK_SDLKMap[vk];
}


static void reset_all_keys()
{
	SDL_Event spoofed_up_event;
	spoofed_up_event.type = SDL_KEYUP;
	spoofed_up_event.key.keysym.unicode = 0;

	for(int i = 0; i < ARRAY_SIZE(keys); i++)
		if(keys[i])
		{
			spoofed_up_event.key.keysym.sym = (SDLKey)i;
			queue_event(spoofed_up_event);
		}
}


static LRESULT OnKey(HWND UNUSED(hWnd), UINT vk, BOOL fDown, int UNUSED(cRepeat), UINT flags)
{
	// TODO Mappings for left/right modifier keys
	// TODO Modifier statekeeping

	const uint sdlk = vkmap(vk);
	if(sdlk != SDLK_UNKNOWN)
		keys[sdlk] = (Uint8)fDown;

	if(!fDown)
		queue_key_event(SDL_KEYUP, sdlk, 0);
	else
	{
		// note: flags is HIWORD(lParam) from WM_KEYDOWN, which includes
		// scancode. ToUnicode also uses its bit 15 to determine if the
		// key is currently pressed.
		const UINT scancode = flags;
		u8 key_states[256];
		GetKeyboardState(key_states);
		WCHAR wchars[8];
		int output_count = ToUnicode(vk, scancode, key_states, wchars, ARRAY_SIZE(wchars), 0);
		// translation succeeded; queue each produced character
		if(output_count > 0)
		{
			for(int i = 0; i < output_count; i++)
				queue_key_event(SDL_KEYDOWN, sdlk, wchars[i]);
		}
		// dead-char; do nothing
		else if(output_count == -1)
		{
		}
		// translation failed; just generate a regular (non-unicode) event
		else if(output_count == 0)
			queue_key_event(SDL_KEYDOWN, sdlk, 0);
		else
			UNREACHABLE;
	}

	return 0;
}


Uint8* SDL_GetKeyState(int* num_keys)
{
	if(num_keys)
		*num_keys = SDLK_LAST;
	return keys;
}


// always on (we don't care about the extra overhead)
int SDL_EnableUNICODE(int UNUSED(enable))
{
	return 1;
}


//----------------------------------------------------------------------------
// mouse

// background: there are several types of coordinates.
// - screen coords are relative to the primary desktop and may therefore be
//   negative on multi-monitor systems (e.g. if secondary monitor is left of
//   primary). they are prefixed with screen_*.
// - "client" coords are simply relative to the parent window's origin and
//   can also be negative (e.g. in the window's NC area).
//   these are prefixed with client_*.
// - "idealized" client coords are what the app sees. these are from
//   0..window dimensions-1. they are derived from client coords that
//   pass the is_in_window() test. unfortunately, these carry different
//   types: we treat them exclusively as uint, while SDL API provides for
//   passing them around as int and packages them into Uint16 in events.

static void queue_mouse_event(uint x, uint y)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEMOTION;
	debug_assert((x|y) <= USHRT_MAX);
	ev.motion.x = (Uint16)x;
	ev.motion.y = (Uint16)y;
	queue_event(ev);
}

static void queue_button_event(uint button, uint state, uint x, uint y)
{
	SDL_Event ev;
	ev.type = (state == SDL_PRESSED)? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
	ev.button.button = (u8)button;
	ev.button.state  = (u8)state;
	debug_assert((x|y) <= USHRT_MAX);
	ev.button.x = (Uint16)x;
	ev.button.y = (Uint16)y;
	queue_event(ev);
}


static uint mouse_x, mouse_y;

// generate a mouse move message and update our notion of the mouse position.
// x, y are client pixel coordinates.
// notes:
// - does not actually move the OS cursor;
// - called from mouse_update and SDL_WarpMouse.
static void mouse_moved(uint x, uint y)
{
	// nothing to do if it hasn't changed since last time
	if(mouse_x == x && mouse_y == y)
		return;

	mouse_x = x;
	mouse_y = y;
	queue_mouse_event(x, y);
}


// convert from screen to client coords.
static void screen_to_client(int screen_x, int screen_y, int& client_x, int& client_y)
{
	POINT pt;
	pt.x = (LONG) screen_x;
	pt.y = (LONG) screen_y;
	// this can fail for unknown reasons even if hWnd is still
	// valid and !is_shutdown. don't WARN_IF_FALSE.
	if(!ScreenToClient(hWnd, &pt))
		pt.x = pt.y = 0;
	client_x = (int)pt.x;
	client_y = (int)pt.y;
}

// are the given coords in our window?
// parameters are client coords as returned by screen_to_client.
// postcondition: it is safe to cast coords to uint and treat them
// as idealized client coords.
static bool is_in_window(int client_x, int client_y)
{
	RECT client_rect;
	WARN_IF_FALSE(GetClientRect(hWnd, &client_rect));
	POINT pt;
	pt.x = (LONG)client_x;
	pt.y = (LONG)client_y;
	if(!PtInRect(&client_rect, pt) || WindowFromPoint(pt) != hWnd)
		return false;

	debug_assert(client_x >= 0 && client_y >= 0);
	return true;
}


static void mouse_update()
{
	// window not created yet or already shut down. no sense reporting
	// mouse position, and bail now to avoid ScreenToClient failing.
	if(hWnd == INVALID_HANDLE_VALUE)
		return;

	// don't use DirectInput, because we want to respect the user's mouse
	// sensitivity settings. Windows messages are laggy, so query current
	// position directly.
	POINT screen_pt;
	WARN_IF_FALSE(GetCursorPos(&screen_pt));
	int client_x, client_y;
	screen_to_client(screen_pt.x, screen_pt.y, client_x, client_y);

	if(is_in_window(client_x, client_y))
	{
		const uint x = (uint)client_x, y = (uint)client_y;

		active_change_state(GAIN, SDL_APPMOUSEFOCUS);
		mouse_moved(x, y);
	}
	// moved outside of window
	else
		active_change_state(LOSE, SDL_APPMOUSEFOCUS);
}

static uint mouse_buttons;

// (we define a new function signature since the windowsx.h message crackers
// don't provide for passing uMsg)
static LRESULT OnMouseButton(HWND UNUSED(hWnd), UINT uMsg, int screen_x, int screen_y, UINT UNUSED(flags))
{
	uint button;
	uint state;
	switch(uMsg)
	{
	case WM_LBUTTONDOWN:
		button = SDL_BUTTON_LEFT;
		state = SDL_PRESSED;
		break;
	case WM_LBUTTONUP:
		button = SDL_BUTTON_LEFT;
		state = SDL_RELEASED;
		break;
	case WM_RBUTTONDOWN:
		button = SDL_BUTTON_RIGHT;
		state = SDL_PRESSED;
		break;
	case WM_RBUTTONUP:
		button = SDL_BUTTON_RIGHT;
		state = SDL_RELEASED;
		break;
	case WM_MBUTTONDOWN:
		button = SDL_BUTTON_MIDDLE;
		state = SDL_PRESSED;
		break;
	case WM_MBUTTONUP:
		button = SDL_BUTTON_MIDDLE;
		state = SDL_RELEASED;
		break;
	NODEFAULT;
	}

	// mouse capture
	static int outstanding_press_events;
	if(state == SDL_PRESSED)
	{
		// grab mouse to ensure we get up events
		if(++outstanding_press_events > 0)
			SetCapture(hWnd);
	}
	else
	{
		// release after all up events received
		if(--outstanding_press_events <= 0)
		{
			ReleaseCapture();
			outstanding_press_events = 0;
		}
	}

	// update button bitfield
	if(state == SDL_PRESSED)
		mouse_buttons |= SDL_BUTTON(button);
	else
		mouse_buttons &= ~SDL_BUTTON(button);

	int client_x, client_y;
	screen_to_client(screen_x, screen_y, client_x, client_y);
	// we may get click events from the NC area or window border where
	// the coords are negative. unfortunately is_in_window can return
	// false due to its window-on-top check, so we better not
	// ignore messages based on that. it is safest to clamp coords to
	// what the app can handle.
	uint x = MAX(client_x, 0), y = MAX(client_y, 0);
	queue_button_event(button, state, x, y);
	return 0;
}


static LRESULT OnMouseWheel(HWND UNUSED(hWnd), int screen_x, int screen_y, int zDelta, UINT UNUSED(fwKeys))
{
	int client_x, client_y;
	screen_to_client(screen_x, screen_y, client_x, client_y);
	// unfortunately we get mouse wheel messages even if mouse is outside
	// our window.
	// to prevent the app from seeing invalid coords, we ignore such messages.
	if(is_in_window(client_x, client_y))
	{
		const uint x = (uint)client_x, y = (uint)client_y;

		uint button = (zDelta < 0)? SDL_BUTTON_WHEELDOWN : SDL_BUTTON_WHEELUP;
		// SDL says this sends a down message followed by up.
		queue_button_event(button, SDL_PRESSED , x, y);
		queue_button_event(button, SDL_RELEASED, x, y);
	}
	return 0;	// handled
}


Uint8 SDL_GetMouseState(int* x, int* y)
{
	if(x)
		*x = (int)mouse_x;
	if(y)
		*y = (int)mouse_y;
	return (Uint8)mouse_buttons;
}


inline void SDL_WarpMouse(int x, int y)
{
	// SDL interface provides for int, but the values should be
	// idealized client coords (>= 0)
	debug_assert(x >= 0 && y >= 0);
	mouse_moved((uint)x, (uint)y);

	POINT screen_pt;
	screen_pt.x = x;
	screen_pt.y = y;
	WARN_IF_FALSE(ClientToScreen(hWnd, &screen_pt));
	WARN_IF_FALSE(SetCursorPos(screen_pt.x, screen_pt.y));
}


int SDL_ShowCursor(int toggle)
{
	static int cursor_visible = SDL_ENABLE;
	if(toggle != SDL_QUERY)
	{
		// only call Windows ShowCursor if changing the visibility -
		// it maintains a counter.
		if(cursor_visible != toggle)
		{
			ShowCursor(toggle);
			cursor_visible = toggle;
		}
	}
	return cursor_visible;
}




//----------------------------------------------------------------------------

static LRESULT CALLBACK wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		// this indicates we allegedly erased the background;
		// PAINTSTRUCT.fErase is then FALSE.
		return 1;

	// prevent selecting menu in fullscreen mode
	case WM_NCHITTEST:
		if(fullscreen)
			return HTCLIENT;
		break;

	HANDLE_MSG(hWnd, WM_ACTIVATE, OnActivate);

	case WM_DESTROY:
		queue_quit_event();
		break;

	case WM_SYSCOMMAND:
		switch(wParam)
		{
		// prevent moving, sizing, screensaver, and power-off in fullscreen mode
		case SC_MOVE:
		case SC_SIZE:
		case SC_MAXIMIZE:
		case SC_MONITORPOWER:
			if(fullscreen)
				return 1;
			break;

		// Alt+F4 or system menu doubleclick/exit
		case SC_CLOSE:
			queue_quit_event();
			break;
		}
		break;

	HANDLE_MSG(hWnd, WM_SYSKEYUP  , OnKey);
	HANDLE_MSG(hWnd, WM_KEYUP     , OnKey);
	HANDLE_MSG(hWnd, WM_SYSKEYDOWN, OnKey);
	HANDLE_MSG(hWnd, WM_KEYDOWN   , OnKey);

	HANDLE_MSG(hWnd, WM_MOUSEWHEEL, OnMouseWheel);

	// (can't use message crackers: they do not provide for passing uMsg)
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		return OnMouseButton(hWnd, uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT)wParam);

	default:
		// can't call DefWindowProc here: some messages
		// are only conditionally 'grabbed' (e.g. NCHITTEST)
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


void SDL_PumpEvents(void)
{
	// rationale: we would like to reduce CPU usage automatically if
	// possible. blocking here until a message arrives would accomplish
	// that, but might potentially freeze the app too long.
	// instead, they should check active state and call SDL_Delay etc.
	// if our window is minimized.

	mouse_update();	// polled

	MSG msg;
	while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
	{
		DispatchMessageW(&msg);
	}
}



int SDL_PollEvent(SDL_Event* ev)
{
	SDL_PumpEvents();

	if(dequeue_event(ev))
		return 1;

	return 0;
}


int SDL_PushEvent(SDL_Event* ev)
{
	queue_event(*ev);
	return 0;
}


//----------------------------------------------------------------------------
// keyboard hook (intercepts PrintScreen and system keys, e.g. Alt+Tab)

// note: the LowLevel hooks are global, but a DLL isn't actually required
// as stated in the docs. Windows apparently calls the handler in its original
// context. see http://www.gamedev.net/community/forums/topic.asp?topic_id=255399 .

static HHOOK hKeyboard_LL_Hook = (HHOOK)0;

static LRESULT CALLBACK keyboard_ll_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if(nCode == HC_ACTION)
	{
		PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
		DWORD vk = p->vkCode;

		// replace Windows PrintScreen handler
		if(vk == VK_SNAPSHOT)
		{
// disabled - we want the normal Windows printscreen handling to
// remain so as not to confuse artists.
#if 0
			// check whether PrintScreen should be taking screenshots -- if
			// not, allow the standard Windows clipboard to work
			if(app_state & SDL_APPINPUTFOCUS)
			{
				// send to wndproc
				UINT msg = (UINT)wParam;
				PostMessage(hWnd, msg, vk, 0);
					// specify hWnd to be safe.
					// if window not yet created, it's INVALID_HANDLE_VALUE.

				// don't pass it on to other handlers
				return 1;
			}
#endif
		}	// vk == VK_SNAPSHOT
	}

	// pass it on to other hook handlers
	return CallNextHookEx(hKeyboard_LL_Hook, nCode, wParam, lParam);
}


static void enable_kbd_hook(bool enable)
{
	if(enable)
	{
		debug_assert(hKeyboard_LL_Hook == 0);
		hKeyboard_LL_Hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_ll_hook, hInst, 0);
		debug_assert(hKeyboard_LL_Hook != 0);
	}
	else
	{
		debug_assert(hKeyboard_LL_Hook != 0);
		UnhookWindowsHookEx(hKeyboard_LL_Hook);
		hKeyboard_LL_Hook = 0;
	}
}


//-----------------------------------------------------------------------------
// byte swapping

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
	const u32 lo = (u32)(x & 0xFFFFFFFF);
	const u32 hi = (u32)(x >> 32);
	u64 ret = SDL_Swap32(lo);
	ret <<= 32;
		// careful: must shift var of type u64, not u32
	ret |= SDL_Swap32(hi);
	return ret;
}
#endif


//-----------------------------------------------------------------------------
// multithread support

// semaphores
// note: implementing these in terms of pthread sem_t doesn't help;
// this wrapper is very close to the Win32 routines.

union HANDLE_sem
{
	HANDLE h;
	SDL_sem* s;
};

SDL_sem* SDL_CreateSemaphore(int cnt)
{
	HANDLE_sem u;
	u.h = CreateSemaphore(0, cnt, 0x7fffffff, 0);
	return u.s;
}

inline void SDL_DestroySemaphore(SDL_sem* sem)
{
	HANDLE_sem u;
	u.s = sem;
	CloseHandle(u.h);
}

int SDL_SemPost(SDL_sem* sem)
{
	HANDLE_sem u;
	u.s = sem;
	return ReleaseSemaphore(u.h, 1, 0);
}

int SDL_SemWait(SDL_sem* sem)
{
	HANDLE_sem u;
	u.s = sem;
	return WaitForSingleObject(u.h, INFINITE);
}


// threads
// users don't need to allocate SDL_Thread variables, so type = void
// API returns SDL_Thread*, which is the HANDLE value itself.
//
// we go through hoops to avoid type cast warnings;
// a simple union { pthread_t; SDL_Thread* } yields "uninitialized"
// warnings in VC2005, so we coerce values directly.
cassert(sizeof(pthread_t) == sizeof(SDL_Thread*));

SDL_Thread* SDL_CreateThread(int (*func)(void*), void* param)
{
	pthread_t thread = 0;
	if(pthread_create(&thread, 0, (void* (*)(void*))func, param) < 0)
		return 0;
	return *(SDL_Thread**)&thread;
}

int SDL_KillThread(SDL_Thread* thread)
{
	pthread_cancel(*(pthread_t*)&thread);
	return 0;
}


//-----------------------------------------------------------------------------
// misc API

void SDL_WM_SetCaption(const char* title, const char* icon)
{
	WARN_IF_FALSE(SetWindowText(hWnd, title));

	// real SDL ignores this parameter, so we will follow suit.
	UNUSED2(icon);
}


inline u32 SDL_GetTicks()
{
	return GetTickCount();
}


inline void SDL_Delay(Uint32 ms)
{
	Sleep(ms);
}


inline void* SDL_GL_GetProcAddress(const char* name)
{
	return wglGetProcAddress(name);
}


//-----------------------------------------------------------------------------
// init/shutdown

static LibError wsdl_init()
{
	hInst = GetModuleHandle(0);

	// redirect stdout to file (otherwise it's simply ignored on Win32).
	// notes:
	// - use full path for safety (works even if someone does chdir)
	// - SDL does this in its WinMain hook. we need to do this here
	//   (before main is called) instead of in SDL_Init to completely
	//   emulate SDL; bonus: we don't miss any output before SDL_Init.
	char path[MAX_PATH];
	snprintf(path, ARRAY_SIZE(path), "%s\\stdout.txt", win_exe_dir);
	// ignore BoundsChecker warnings here. subsystem is set to "Windows"
	// to avoid the OS opening a console on startup (ugly). that means
	// stdout isn't associated with a lowio handle; _close ends up
	// getting called with fd = -1. oh well, nothing we can do.
	FILE* f = freopen(path, "wt", stdout);
	if(!f)
		debug_warn("stdout freopen failed");

#if CONFIG_PARANOIA
	// disable buffering, so that no writes are lost even if the program
	// crashes. only enabled in full debug mode because this is really slow!
	setvbuf(stdout, 0, _IONBF, 0);
#endif

	enable_kbd_hook(true);

	return INFO::OK;
}


static LibError wsdl_shutdown()
{
	is_shutdown = true;

	// redirected to stdout.txt in SDL_Init;
	// close to avoid BoundsChecker warning.
	fclose(stdout);

	gamma_swap(GAMMA_RESTORE_ORIGINAL);

	video_shutdown();

	enable_kbd_hook(false);

	return INFO::OK;
}


// these are placebos. since some init needs to happen before main(),
// we take care of it all in the module init/shutdown hooks.

int SDL_Init(Uint32 UNUSED(flags))
{
	return 0;
}

void SDL_Quit()
{
}
