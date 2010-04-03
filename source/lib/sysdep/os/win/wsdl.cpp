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

#include "precompiled.h"
#include "lib/external_libraries/sdl.h"

#if CONFIG2_WSDL

#include <stdio.h>
#include <math.h>
#include <queue>
#include <algorithm>

#include "lib/sysdep/os/win/win.h"
#include <process.h>	// _beginthreadex
#include <WindowsX.h>	// message crackers

#include "lib/posix/posix_pthread.h"
#include "lib/module_init.h"
#include "lib/sysdep/os/win/wutil.h"
#include "lib/sysdep/os/win/winit.h"
#include "lib/sysdep/os/win/wmi.h"	// for SDL_GetVideoInfo

#if MSC_VERSION
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

#include "lib/ogl.h"		// needed to pull in the delay-loaded opengl32.dll

WINIT_REGISTER_LATE_INIT(wsdl_Init);
WINIT_REGISTER_EARLY_SHUTDOWN(wsdl_Shutdown);


// in fullscreen mode, i.e. not windowed.
// video mode will be restored when app is deactivated.
static bool fullscreen;

// the app is shutting down.
// if set, ignore further Windows messages for clean shutdown.
static bool is_quitting;

static HWND g_hWnd = (HWND)INVALID_HANDLE_VALUE;
static HDC g_hDC = (HDC)INVALID_HANDLE_VALUE;	// needed by gamma code


//----------------------------------------------------------------------------
// gamma

class GammaRamp
{
public:
	GammaRamp()
		: m_hasChanged(false)
	{
	}

	bool Change(float gamma_r, float gamma_g, float gamma_b)
	{
		// get current ramp (once) so we can later restore it.
		if(!m_hasChanged)
		{
			debug_assert(wutil_IsValidHandle(g_hDC));
			if(!GetDeviceGammaRamp(g_hDC, m_original))
				return false;
		}

		Compute(gamma_r, m_changed+0*256);
		Compute(gamma_g, m_changed+1*256);
		Compute(gamma_b, m_changed+2*256);
		if(!Upload(m_changed))
			return false;

		m_hasChanged = true;
		return true;
	}

	void Latch()
	{
		if(m_hasChanged)
			Upload(m_changed);
	}

	void RestoreOriginal()
	{
		if(m_hasChanged)
			Upload(m_original);
	}

private:
	static void Compute(float gamma, u16* ramp)
	{
		// assume identity if invalid
		if(gamma <= 0.0f)
			gamma = 1.0f;

		// identity: special-case to make sure we get exact values
		if(gamma == 1.0f)
		{
			for(u16 i = 0; i < 256; i++)
				ramp[i] = u16(i << 8);
			return;
		}

		for(int i = 0; i < 256; i++)
		{
			const double val = pow(i/255.0, (double)gamma);
			const double clamped = std::max(0.0, std::min(val, 1.0-DBL_EPSILON));
			ramp[i] = u16_from_double(clamped);
		}
		debug_assert(ramp[0] == 0);
		debug_assert(ramp[255] == 0xFFFF);
	}

	bool Upload(u16* ramps)
	{
		WinScopedPreserveLastError s;
		SetLastError(0);
		debug_assert(wutil_IsValidHandle(g_hDC));
		BOOL ok = SetDeviceGammaRamp(g_hDC, ramps);
		// on multi-monitor NVidia systems, the first call after a reboot
		// fails, but subsequent ones succeed.
		// see http://icculus.org/pipermail/quake3-bugzilla/2009-October/001316.html
		if(ok == FALSE)
		{
			ok = SetDeviceGammaRamp(g_hDC, ramps);
			debug_assert(ok);
		}
		return (ok == TRUE);
	}

	bool m_hasChanged;

	// values are 8.8 fixed point
	u16 m_original[3*256];
	u16 m_changed[3*256];
};

static GammaRamp gammaRamp;


// note: any component gamma = 0 is assumed to be identity.
int SDL_SetGamma(float r, float g, float b)
{
	return gammaRamp.Change(r, g, b)? 0 : -1;
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
	ShowWindow(g_hWnd, SW_RESTORE);
	ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
}

static inline void video_leave_game_mode()
{
	ChangeDisplaySettings(0, 0);
	ShowWindow(g_hWnd, SW_MINIMIZE);
}


int SDL_GL_SetAttribute(SDL_GLattr attr, int value)
{
	if(attr == SDL_GL_DEPTH_SIZE)
		depth_bits = value;

	return 0;
}


static LRESULT CALLBACK wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static HWND wsdl_CreateWindow(int w, int h)
{
	// (create new window every time (instead of once at startup), because
	// pixel format isn't supposed to be changed more than once)

	// app instance.
	// returned by GetModuleHandle and used in kbd hook and window creation. 
	const HINSTANCE hInst = GetModuleHandle(0);

	// register window class
	WNDCLASSW wc;
	memset(&wc, 0, sizeof(wc));
	wc.style = 0;
	wc.lpfnWndProc = wndproc;
	wc.lpszClassName = L"WSDL";
	wc.hInstance = hInst;
	ATOM class_atom = RegisterClassW(&wc);
	if(!class_atom)
	{
		debug_assert(0);	// SDL_SetVideoMode: RegisterClass failed
		return 0;
	}

	DWORD windowStyle = fullscreen? WS_POPUP : WS_POPUPWINDOW|WS_CAPTION|WS_MINIMIZEBOX;
	windowStyle |= WS_VISIBLE;
	windowStyle |= WS_CLIPCHILDREN|WS_CLIPSIBLINGS;	// MSDN SetPixelFormat says this is required

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
	return CreateWindowExW(WS_EX_APPWINDOW, (LPCWSTR)(uintptr_t)class_atom, L"wsdl", windowStyle, 0, 0, w, h, 0, 0, hInst, 0);
}


static void SetPixelFormat(HDC g_hDC, int bpp)
{
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

	// note: the GDI pixel format functions require opengl32.dll to be loaded.
	// a deadlock on the next line is probably due to VLD's LdrLoadDll hook.

	const int pf = ChoosePixelFormat(g_hDC, &pfd);
	debug_assert(pf >= 1);
	WARN_IF_FALSE(SetPixelFormat(g_hDC, pf, &pfd));
}


// set video mode wxh:bpp if necessary.
// w = h = bpp = 0 => no change.
int SDL_SetVideoMode(int w, int h, int bpp, unsigned long flags)
{
	WinScopedPreserveLastError s;	// OpenGL and GDI

	fullscreen = (flags & SDL_FULLSCREEN) != 0;

	// get current mode settings
	memset(&dm, 0, sizeof(dm));
	dm.dmSize = sizeof(dm);
	EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm);
	const int cur_w = (int)dm.dmPelsWidth, cur_h = (int)dm.dmPelsHeight;

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

	g_hWnd = wsdl_CreateWindow(w, h);
	if(!wutil_IsValidHandle(g_hWnd))
		return 0;

	g_hDC = GetDC(g_hWnd);

	SetPixelFormat(g_hDC, bpp);

	hGLRC = wglCreateContext(g_hDC);
	if(!hGLRC)
		return 0;

	if(!wglMakeCurrent(g_hDC, hGLRC))
		return 0;

	return 1;
}


static void video_shutdown()
{
	if(fullscreen)
	{
		LONG status = ChangeDisplaySettings(0, 0);
		debug_assert(status == DISP_CHANGE_SUCCESSFUL);
	}

	if(hGLRC != INVALID_HANDLE_VALUE)
	{
		WARN_IF_FALSE(wglMakeCurrent(0, 0));
		WARN_IF_FALSE(wglDeleteContext(hGLRC));
		hGLRC = (HGLRC)INVALID_HANDLE_VALUE;
	}
}


void SDL_GL_SwapBuffers()
{
	SwapBuffers(g_hDC);
}


SDL_VideoInfo* SDL_GetVideoInfo()
{
	static SDL_VideoInfo video_info;

	if(video_info.video_mem == 0)
	{
		WmiMap videoAdapter;
		if(wmi_GetClass(L"Win32_VideoController", videoAdapter) == INFO::OK)
		{
			VARIANT vTotalMemory = videoAdapter[L"AdapterRAM"];
			video_info.video_mem = vTotalMemory.lVal;
		}
	}

	return &video_info;
}


SDL_Surface* SDL_GetVideoSurface()
{
	return 0;
}


//----------------------------------------------------------------------------
// event queue

// note: we only use winit to redirect stdout; this queue won't be used
// before _cinit.
typedef std::queue<SDL_Event> Queue;
static Queue queue;

static void queue_event(const SDL_Event& ev)
{
	debug_assert(!is_quitting);

	queue.push(ev);
}

static bool dequeue_event(SDL_Event* ev)
{
	debug_assert(!is_quitting);

	if(queue.empty())
		return false;
	*ev = queue.front();
	queue.pop();
	return true;
}


//----------------------------------------------------------------------------
// app activation

enum SdlActivationType { LOSE = 0, GAIN = 1 };

static inline void queue_active_event(SdlActivationType type, size_t changed_app_state)
{
	// SDL says this event is not generated when the window is created,
	// but skipping the first event may confuse things.

	SDL_Event ev;
	ev.type = SDL_ACTIVEEVENT;
	ev.active.state = (u8)changed_app_state;
	ev.active.gain  = (u8)((type == GAIN)? 1 : 0);
	queue_event(ev);
}


// SDL_APP* bitflags indicating whether we are active.
// note: responsibility for yielding lies with SDL apps -
// they control the main loop.
static Uint8 app_state;

static void active_change_state(SdlActivationType type, Uint8 changed_app_state)
{
	Uint8 old_app_state = app_state;

	if(type == GAIN)
		app_state = Uint8(app_state | changed_app_state);
	else
		app_state = Uint8(app_state & ~changed_app_state);

	// generate an event - but only if the given state flags actually changed.
	if((old_app_state & changed_app_state) != (app_state & changed_app_state))
		queue_active_event(type, changed_app_state);
}

static void reset_all_keys();

static LRESULT OnActivate(HWND hWnd, UINT state, HWND UNUSED(hWndActDeact), BOOL fMinimized)
{
	SdlActivationType type;
	Uint8 changed_app_state;

	// went active and not minimized
	if(state != WA_INACTIVE && !fMinimized)
	{
		type = GAIN;
		changed_app_state = SDL_APPINPUTFOCUS|SDL_APPACTIVE;

		// grab keyboard focus (we previously had DefWindowProc do this).
		SetFocus(hWnd);

		gammaRamp.Latch();
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

		gammaRamp.RestoreOriginal();
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


static void queue_quit_event()
{
	SDL_Event ev;
	ev.type = SDL_QUIT;
	queue_event(ev);
}


//----------------------------------------------------------------------------
// keyboard

// note: keysym.unicode is only returned for SDL_KEYDOWN, and is otherwise 0.
static void queue_key_event(Uint8 type, SDLKey sdlk, WCHAR unicode_char)
{
	SDL_Event ev;
	ev.type = type;
	ev.key.keysym.sym     = sdlk;
	ev.key.keysym.unicode = (Uint16)unicode_char;
	queue_event(ev);
}


static Uint8 keys[SDLK_LAST];

// winuser.h promises VK_0..9 and VK_A..Z etc. match ASCII value.
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
		debug_assert(0);	// invalid vk
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

	const SDLKey sdlk = vkmap(vk);
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
// - "idealized" coords are what the app sees. these range from 0 to
//   windowDimensions-1. they are returned by GetCoords and have no prefix.

static void queue_mouse_event(int x, int y)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEMOTION;
	debug_assert(unsigned(x|y) <= USHRT_MAX);
	ev.motion.x = (Uint16)x;
	ev.motion.y = (Uint16)y;
	queue_event(ev);
}

static void queue_button_event(int button, int state, int x, int y)
{
	SDL_Event ev;
	ev.type = Uint8((state == SDL_PRESSED)? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP);
	ev.button.button = (u8)button;
	ev.button.state  = (u8)state;
	debug_assert(unsigned(x|y) <= USHRT_MAX);
	ev.button.x = (Uint16)x;
	ev.button.y = (Uint16)y;
	queue_event(ev);
}


static int mouse_x, mouse_y;

// generate a mouse move message and update our notion of the mouse position.
// x, y are client pixel coordinates.
// notes:
// - does not actually move the OS cursor;
// - called from mouse_update and SDL_WarpMouse.
static void mouse_moved(int x, int y)
{
	// nothing to do if it hasn't changed since last time
	if(mouse_x == x && mouse_y == y)
		return;

	mouse_x = x;
	mouse_y = y;
	queue_mouse_event(x, y);
}

static POINT ScreenFromClient(int client_x, int client_y)
{
	POINT screen_pt;
	screen_pt.x = (LONG)client_x;
	screen_pt.y = (LONG)client_y;
	WARN_IF_FALSE(ClientToScreen(g_hWnd, &screen_pt));
	return screen_pt;
}

// get idealized client coordinates or return false if outside our window.
static bool GetCoords(int screen_x, int screen_y, int& x, int& y)
{
	debug_assert(wutil_IsValidHandle(g_hWnd));

	POINT screen_pt;
	screen_pt.x = (LONG)screen_x;
	screen_pt.y = (LONG)screen_y;

	POINT client_pt;
	{
		// note: MapWindowPoints has a really stupid interface, returning 0
		// on failure or if no shift was needed (i.e. window is fullscreen).
		// we must use GetLastError to detect error conditions.
		WinScopedPreserveLastError s;
		SetLastError(0);
		client_pt = screen_pt;	// translated below
		const int ret = MapWindowPoints(HWND_DESKTOP, g_hWnd, &client_pt, 1);
		debug_assert(ret != 0 || GetLastError() == 0);
	}

	{
		RECT client_rect;
		WARN_IF_FALSE(GetClientRect(g_hWnd, &client_rect));
		if(!PtInRect(&client_rect, client_pt))
			return false;
	}

	if(WindowFromPoint(screen_pt) != g_hWnd)
		return false;

	x = client_pt.x;
	y = client_pt.y;
	debug_assert(x >= 0 && y >= 0);
	return true;
}


static void mouse_update()
{
	// window not created yet or already shut down. no sense reporting
	// mouse position, and bail now to avoid ScreenToClient failing.
	if(!wutil_IsValidHandle(g_hWnd))
		return;

	// don't use DirectInput, because we want to respect the user's mouse
	// sensitivity settings. Windows messages are laggy, so query current
	// position directly.
	// note: GetCursorPos fails if the desktop is switched (e.g. after
	// pressing Ctrl+Alt+Del), which can be ignored.
	POINT screen_pt;
	if(!GetCursorPos(&screen_pt))
		return;
	int x, y;
	if(GetCoords(screen_pt.x, screen_pt.y, x, y))
	{
		active_change_state(GAIN, SDL_APPMOUSEFOCUS);
		mouse_moved(x, y);
	}
	// moved outside of window
	else
		active_change_state(LOSE, SDL_APPMOUSEFOCUS);
}

static unsigned mouse_buttons;

// (we define a new function signature since the windowsx.h message crackers
// don't provide for passing uMsg)
static LRESULT OnMouseButton(HWND UNUSED(hWnd), UINT uMsg, int client_x, int client_y, UINT UNUSED(flags))
{
	int button;
	int state;
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
			SetCapture(g_hWnd);
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

	const POINT screen_pt = ScreenFromClient(client_x, client_y);
	int x, y;
	if(GetCoords(screen_pt.x, screen_pt.y, x, y))
		queue_button_event(button, state, x, y);
	return 0;
}


// (note: this message is sent even if the cursor is outside our window)
static LRESULT OnMouseWheel(HWND UNUSED(hWnd), int screen_x, int screen_y, int zDelta, UINT UNUSED(fwKeys))
{
	int x, y;
	if(GetCoords(screen_x, screen_y, x, y))
	{
		int button = (zDelta < 0)? SDL_BUTTON_WHEELDOWN : SDL_BUTTON_WHEELUP;
		// SDL says this sends a down message followed by up.
		queue_button_event(button, SDL_PRESSED,  x, y);
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
	mouse_moved(x, y);

	const int client_x = x, client_y = y;
	const POINT screen_pt = ScreenFromClient(client_x, client_y);
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

static LRESULT OnDestroy(HWND hWnd)
{
	debug_assert(hWnd == g_hWnd);
	WARN_IF_FALSE(ReleaseDC(g_hWnd, g_hDC));
	g_hDC = (HDC)INVALID_HANDLE_VALUE;
	g_hWnd = (HWND)INVALID_HANDLE_VALUE;
	queue_quit_event();
	PostQuitMessage(0);

#ifdef _DEBUG
	// see http://www.adrianmccarthy.com/blog/?p=51
	// with WM_QUIT in the message queue, MessageBox will immediately
	// return IDABORT. to ensure any subsequent CRT error reports are
	// at least somewhat visible, we redirect them to debug output.
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif

	return 0;
}


static LRESULT CALLBACK wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(is_quitting)
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
	HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);

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


void SDL_PumpEvents()
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

static HANDLE HANDLE_from_sem(SDL_sem* s)
{
	return (HANDLE)s;
}

static SDL_sem* sem_from_HANDLE(HANDLE h)
{
	return (SDL_sem*)h;
}

SDL_sem* SDL_CreateSemaphore(int cnt)
{
	HANDLE h = CreateSemaphore(0, cnt, 0x7fffffff, 0);
	return sem_from_HANDLE(h);
}

inline void SDL_DestroySemaphore(SDL_sem* sem)
{
	HANDLE h = HANDLE_from_sem(sem);
	CloseHandle(h);
}

int SDL_SemPost(SDL_sem* sem)
{
	HANDLE h = HANDLE_from_sem(sem);
	return ReleaseSemaphore(h, 1, 0);
}

int SDL_SemWait(SDL_sem* sem)
{
	HANDLE h = HANDLE_from_sem(sem);
	return WaitForSingleObject(h, INFINITE);
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
	WARN_IF_FALSE(SetWindowTextA(g_hWnd, title));

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

// defend against calling SDL_Quit twice (GameSetup does this to work
// around ATI driver breakage)
static ModuleInitState initState;

int SDL_Init(Uint32 UNUSED(flags))
{
	if(!ModuleShouldInitialize(&initState))
		return 0;

	return 0;
}

void SDL_Quit()
{
	if(!ModuleShouldShutdown(&initState))
		return;

	is_quitting = true;

	if(g_hDC != INVALID_HANDLE_VALUE)
		gammaRamp.RestoreOriginal();

	if(g_hWnd != INVALID_HANDLE_VALUE)
		WARN_IF_FALSE(DestroyWindow(g_hWnd));

	video_shutdown();
}

 
static void RedirectStdout()
{
	// this process is apparently attached to a console, and users might be
	// surprised to find that we redirected the output to a file, so don't.
	if(GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
		return;

	// notes:
	// - the current directory may have changed, so use the full path
	//   to the current executable.
	// - add the EXE name to the filename to allow multiple executables
	//   with their own redirections.
	wchar_t pathnameEXE[MAX_PATH];
	const DWORD charsWritten = GetModuleFileNameW(0, pathnameEXE, ARRAY_SIZE(pathnameEXE));
	debug_assert(charsWritten);
	const fs::wpath path = fs::wpath(pathnameEXE).branch_path();
	std::wstring name = fs::basename(pathnameEXE);
	fs::wpath pathname(path/(name+L"_stdout.txt"));
 	// ignore BoundsChecker warnings here. subsystem is set to "Windows"
 	// to avoid the OS opening a console on startup (ugly). that means
 	// stdout isn't associated with a lowio handle; _close ends up
 	// getting called with fd = -1. oh well, nothing we can do.
 	FILE* f = 0;
	errno_t ret = _wfreopen_s(&f, pathname.string().c_str(), L"wt", stdout);
 	debug_assert(ret == 0);
 	debug_assert(f);
 
#if CONFIG_PARANOIA
	// disable buffering, so that no writes are lost even if the program
 	// crashes. only enabled in full debug mode because this is really slow!
 	setvbuf(stdout, 0, _IONBF, 0);
 #endif
}
 
static LibError wsdl_Init()
{
	// note: SDL does this in its WinMain hook. doing so in SDL_Init would be
	// too late (we might miss some output), so we use winit.
	// (this is possible because _cinit has already been called)
	RedirectStdout();

 	return INFO::OK;
}

static LibError wsdl_Shutdown()
{
	// was redirected to stdout.txt; closing avoids a BoundsChecker warning.
	fclose(stdout);

	return INFO::OK;
}

#endif	// #if CONFIG2_WSDL
