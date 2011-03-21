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

#include "lib/module_init.h"
#include "lib/path_util.h"
#include "lib/posix/posix_pthread.h"
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

// usable at any time (required by SDL_SetGamma); this is made
// mostly safe via CS_OWNDC.
static HDC g_hDC = (HDC)INVALID_HANDLE_VALUE;


//----------------------------------------------------------------------------
// gamma

class GammaRamp
{
public:
	GammaRamp()
		: m_hasChanged(false)
	{
	}

	bool Change(HDC hDC, float gamma_r, float gamma_g, float gamma_b)
	{
		// get current ramp (once) so we can later restore it.
		if(!m_hasChanged)
		{
			debug_assert(wutil_IsValidHandle(hDC));
			if(!GetDeviceGammaRamp(hDC, m_original))
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
	return gammaRamp.Change(g_hDC, r, g, b)? 0 : -1;
}


//----------------------------------------------------------------------------
// window
//----------------------------------------------------------------------------

static DWORD wnd_ChooseWindowStyle(bool fullscreen, HWND previousWindow = (HWND)INVALID_HANDLE_VALUE)
{
	DWORD windowStyle = fullscreen? WS_POPUP : WS_POPUPWINDOW|WS_CAPTION|WS_MINIMIZEBOX;
	windowStyle |= WS_VISIBLE;
	windowStyle |= WS_CLIPCHILDREN|WS_CLIPSIBLINGS;	// MSDN SetPixelFormat says this is required

	if(!fullscreen)	// windowed
	{
		// support resizing
		windowStyle |= WS_SIZEBOX|WS_MAXIMIZEBOX;

		// remember the previous maximized state
		// (else subsequent attempts to maximize will fail)
		if(wutil_IsValidHandle(previousWindow))
		{
			const DWORD previousWindowState = GetWindowLongW(previousWindow, GWL_STYLE);
			windowStyle |= (previousWindowState & WS_MAXIMIZE);
		}
	}

	return windowStyle;
}


// @param w,h value-return (in: desired, out: actual pixel count)
static void wnd_UpdateWindowDimensions(DWORD windowStyle, int& w, int& h)
{
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
}


static LRESULT CALLBACK OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static HWND wnd_CreateWindow(int w, int h)
{
	// (create new window every time (instead of once at startup), because
	// pixel format isn't supposed to be changed more than once)

	// app instance.
	// returned by GetModuleHandle and used in keyboard hook and window creation. 
	const HINSTANCE hInst = GetModuleHandle(0);

	// register window class
	WNDCLASSW wc;
	memset(&wc, 0, sizeof(wc));
	// no CS_VREDRAW and CS_HREDRAW - avoids redrawing when resized.
	wc.style = CS_OWNDC;	// (see g_hDC definition)
	wc.lpfnWndProc = OnMessage;
	wc.lpszClassName = L"WSDL";
	wc.hInstance = hInst;
	ATOM class_atom = RegisterClassW(&wc);
	if(!class_atom)
	{
		debug_assert(0);	// RegisterClassW failed
		return 0;
	}

	const DWORD windowStyle = wnd_ChooseWindowStyle(fullscreen);
	wnd_UpdateWindowDimensions(windowStyle, w, h);

	// note: you can override the hardcoded window name via SDL_WM_SetCaption.
	return CreateWindowExW(WS_EX_APPWINDOW, (LPCWSTR)(uintptr_t)class_atom, L"wsdl", windowStyle, 0, 0, w, h, 0, 0, hInst, 0);
}


//----------------------------------------------------------------------------
// video
//----------------------------------------------------------------------------

static DEVMODE dm;			// current video mode
static HGLRC hGLRC = (HGLRC)INVALID_HANDLE_VALUE;

// set via SDL_GL_SetAttribute:
static int depthBufferBits = 24;
static int stencilBufferBits = 0;
static int vsyncEnabled = 1;

int SDL_GL_SetAttribute(SDL_GLattr attr, int value)
{
	switch(attr)
	{
	case SDL_GL_DEPTH_SIZE:
		depthBufferBits = value;
		break;

	case SDL_GL_STENCIL_SIZE:
		stencilBufferBits = value;
		break;

	case SDL_GL_SWAP_CONTROL:
		vsyncEnabled = value;
		break;

	case SDL_GL_DOUBLEBUFFER:
		// (always enabled)
		break;
	}

	return 0;
}


// check if resolution needs to be changed
static bool video_NeedsChange(int w, int h, int cur_w, int cur_h, bool fullscreen)
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


static void video_SetPixelFormat(HDC g_hDC, int bpp)
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
	const BYTE cDepthBits   = (BYTE)depthBufferBits;
	const BYTE cStencilBits = (BYTE)stencilBufferBits;
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


// set video mode width x height : bpp (or leave unchanged if already adequate).
// w = h = bpp = 0 => no change.
SDL_Surface* SDL_SetVideoMode(int w, int h, int bpp, Uint32 flags)
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

	if(video_NeedsChange(w,h, cur_w,cur_h, fullscreen))
	{
		dm.dmPelsWidth  = (DWORD)w;
		dm.dmPelsHeight = (DWORD)h;
		dm.dmFields |= DM_PELSWIDTH|DM_PELSHEIGHT;
	}
	// the (possibly changed) mode will be (re)set at next WM_ACTIVATE

	if(g_hWnd == (HWND)INVALID_HANDLE_VALUE)
	{
		g_hWnd = wnd_CreateWindow(w, h);
		if(!wutil_IsValidHandle(g_hWnd))
			return 0;

		g_hDC = GetDC(g_hWnd);

		video_SetPixelFormat(g_hDC, bpp);

		hGLRC = wglCreateContext(g_hDC);
		if(!hGLRC)
			return 0;

		if(!wglMakeCurrent(g_hDC, hGLRC))
			return 0;
	}
	else	// update the existing window
	{
		const DWORD windowStyle = wnd_ChooseWindowStyle(fullscreen, g_hWnd);
		wnd_UpdateWindowDimensions(windowStyle, w, h);

		UINT swp_flags = SWP_FRAMECHANGED|SWP_NOZORDER|SWP_NOACTIVATE;
		if(!fullscreen)	// windowed: preserve the top-left corner
			swp_flags |= SWP_NOMOVE;

		WARN_IF_FALSE(SetWindowLongW(g_hWnd, GWL_STYLE, windowStyle));
		WARN_IF_FALSE(SetWindowPos(g_hWnd, 0, 0, 0, w, h, swp_flags));

		if(fullscreen)
		{
			ShowWindow(g_hWnd, SW_RESTORE);
			ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
		}
		else
		{
			ChangeDisplaySettings(0, 0);
			// don't ShowWindow with SW_MINIMIZE (we just want to update)
		}
	}

	// get the actual updated window size and return it
	static SDL_Surface screen;
	RECT rect;
	WARN_IF_FALSE(GetClientRect(g_hWnd, &rect));
	screen.w = rect.right;
	screen.h = rect.bottom;

	// (required for ogl_HaveExtension, but callers should also invoke
	// ogl_Init in case the real SDL is being used.)
	ogl_Init();
	if(ogl_HaveExtension("WGL_EXT_swap_control") && pwglSwapIntervalEXT)
		pwglSwapIntervalEXT(vsyncEnabled);

	return &screen;
}


static void video_Shutdown()
{
	if(fullscreen)
	{
		LONG status = ChangeDisplaySettings(0, 0);
		debug_assert(status == DISP_CHANGE_SUCCESSFUL);
	}

	if(wutil_IsValidHandle(hGLRC))
	{
		WARN_IF_FALSE(wglMakeCurrent(0, 0));
		WARN_IF_FALSE(wglDeleteContext(hGLRC));
		hGLRC = (HGLRC)INVALID_HANDLE_VALUE;
	}

	g_hWnd = (HWND)INVALID_HANDLE_VALUE;
	g_hDC = (HDC)INVALID_HANDLE_VALUE;
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
static Queue g_queue;

static void QueueEvent(const SDL_Event& ev)
{
	g_queue.push(ev);
}

static bool DequeueEvent(SDL_Event& ev)
{
	if(g_queue.empty())
		return false;
	ev = g_queue.front();
	g_queue.pop();
	return true;
}


//----------------------------------------------------------------------------
// keyboard

// note: keysym.unicode is only returned for SDL_KEYDOWN, and is otherwise 0.
static void QueueKeyEvent(Uint8 type, SDLKey sdlk, WCHAR unicode_char)
{
	SDL_Event ev;
	ev.type = type;
	ev.key.keysym.sym = sdlk;
	ev.key.keysym.unicode = (Uint16)unicode_char;
	QueueEvent(ev);
}


static Uint8 keys[SDLK_LAST];

static SDLKey g_SDLKeyForVK[256]; // g_SDLKeyForVK[vk] == SDLK

static void key_Init()
{
	// Map the VK keysyms
	for(size_t i = 0; i < ARRAY_SIZE(g_SDLKeyForVK); i++)
		g_SDLKeyForVK[i] = SDLK_UNKNOWN;

	g_SDLKeyForVK[VK_BACK] = SDLK_BACKSPACE;
	g_SDLKeyForVK[VK_TAB] = SDLK_TAB;
	g_SDLKeyForVK[VK_CLEAR] = SDLK_CLEAR;
	g_SDLKeyForVK[VK_RETURN] = SDLK_RETURN;
	g_SDLKeyForVK[VK_PAUSE] = SDLK_PAUSE;
	g_SDLKeyForVK[VK_ESCAPE] = SDLK_ESCAPE;
	g_SDLKeyForVK[VK_SPACE] = SDLK_SPACE;
	g_SDLKeyForVK[VK_OEM_7] = SDLK_QUOTE;
	g_SDLKeyForVK[VK_OEM_COMMA] = SDLK_COMMA;
	g_SDLKeyForVK[VK_OEM_MINUS] = SDLK_MINUS;
	g_SDLKeyForVK[VK_OEM_PERIOD] = SDLK_PERIOD;
	g_SDLKeyForVK[VK_OEM_2] = SDLK_SLASH;
	g_SDLKeyForVK[VK_OEM_1] = SDLK_SEMICOLON;
	g_SDLKeyForVK[VK_OEM_PLUS] = SDLK_EQUALS;
	g_SDLKeyForVK[VK_OEM_4] = SDLK_LEFTBRACKET;
	g_SDLKeyForVK[VK_OEM_5] = SDLK_BACKSLASH;
	g_SDLKeyForVK[VK_OEM_6] = SDLK_RIGHTBRACKET;
	g_SDLKeyForVK[VK_OEM_3] = SDLK_BACKQUOTE;
	g_SDLKeyForVK[VK_OEM_8] = SDLK_BACKQUOTE;

	// winuser.h guarantees A..Z and 0..9 match their ASCII values:
	const int VK_0 = '0';
	for(int i = 0; i < 10; i++)
		g_SDLKeyForVK[VK_0+i] = (SDLKey)(SDLK_0+i);
	const int VK_A = 'A';
	for(int i = 0; i < 26; i++)
		g_SDLKeyForVK[VK_A+i] = (SDLKey)(SDLK_a+i);

	g_SDLKeyForVK[VK_DELETE] = SDLK_DELETE;

	for(int i = 0; i < 10; i++)
		g_SDLKeyForVK[VK_NUMPAD0+i] = (SDLKey)(SDLK_KP0+i);

	g_SDLKeyForVK[VK_DECIMAL] = SDLK_KP_PERIOD;
	g_SDLKeyForVK[VK_DIVIDE] = SDLK_KP_DIVIDE;
	g_SDLKeyForVK[VK_MULTIPLY] = SDLK_KP_MULTIPLY;
	g_SDLKeyForVK[VK_SUBTRACT] = SDLK_KP_MINUS;
	g_SDLKeyForVK[VK_ADD] = SDLK_KP_PLUS;

	g_SDLKeyForVK[VK_UP] = SDLK_UP;
	g_SDLKeyForVK[VK_DOWN] = SDLK_DOWN;
	g_SDLKeyForVK[VK_RIGHT] = SDLK_RIGHT;
	g_SDLKeyForVK[VK_LEFT] = SDLK_LEFT;
	g_SDLKeyForVK[VK_INSERT] = SDLK_INSERT;
	g_SDLKeyForVK[VK_HOME] = SDLK_HOME;
	g_SDLKeyForVK[VK_END] = SDLK_END;
	g_SDLKeyForVK[VK_PRIOR] = SDLK_PAGEUP;
	g_SDLKeyForVK[VK_NEXT] = SDLK_PAGEDOWN;

	for(int i = 0; i < 12; i++)
		g_SDLKeyForVK[VK_F1+i] = (SDLKey)(SDLK_F1+i);

	g_SDLKeyForVK[VK_NUMLOCK] = SDLK_NUMLOCK;
	g_SDLKeyForVK[VK_CAPITAL] = SDLK_CAPSLOCK;
	g_SDLKeyForVK[VK_SCROLL] = SDLK_SCROLLOCK;
	g_SDLKeyForVK[VK_RSHIFT] = SDLK_RSHIFT;
	g_SDLKeyForVK[VK_LSHIFT] = SDLK_LSHIFT;
	g_SDLKeyForVK[VK_SHIFT] = SDLK_LSHIFT; // XXX: Not quite
	g_SDLKeyForVK[VK_RCONTROL] = SDLK_RCTRL;
	g_SDLKeyForVK[VK_LCONTROL] = SDLK_LCTRL;
	g_SDLKeyForVK[VK_CONTROL] = SDLK_LCTRL; // XXX: Not quite
	g_SDLKeyForVK[VK_RMENU] = SDLK_RALT;
	g_SDLKeyForVK[VK_LMENU] = SDLK_LALT;
	g_SDLKeyForVK[VK_MENU] = SDLK_LALT; // XXX: Not quite
	g_SDLKeyForVK[VK_RWIN] = SDLK_RSUPER;
	g_SDLKeyForVK[VK_LWIN] = SDLK_LSUPER;

	g_SDLKeyForVK[VK_HELP] = SDLK_HELP;
#ifdef VK_PRINT
	g_SDLKeyForVK[VK_PRINT] = SDLK_PRINT;
#endif
	g_SDLKeyForVK[VK_SNAPSHOT] = SDLK_PRINT;
	g_SDLKeyForVK[VK_CANCEL] = SDLK_BREAK;
	g_SDLKeyForVK[VK_APPS] = SDLK_MENU;
}


static inline SDLKey SDLKeyFromVK(int vk)
{
	if(!(0 <= vk && vk < 256))
	{
		debug_assert(0);	// invalid vk
		return SDLK_UNKNOWN;
	}
	return g_SDLKeyForVK[vk];
}


static void key_ResetAll()
{
	SDL_Event spoofed_up_event;
	spoofed_up_event.type = SDL_KEYUP;
	spoofed_up_event.key.keysym.unicode = 0;

	for(size_t i = 0; i < ARRAY_SIZE(keys); i++)
	{
		if(keys[i])
		{
			spoofed_up_event.key.keysym.sym = (SDLKey)i;
			QueueEvent(spoofed_up_event);
		}
	}
}


static LRESULT OnKey(HWND UNUSED(hWnd), UINT vk, BOOL fDown, int UNUSED(cRepeat), UINT flags)
{
	// TODO Mappings for left/right modifier keys
	// TODO Modifier statekeeping

	const SDLKey sdlk = SDLKeyFromVK(vk);
	if(sdlk != SDLK_UNKNOWN)
		keys[sdlk] = (Uint8)fDown;

	if(!fDown)
		QueueKeyEvent(SDL_KEYUP, sdlk, 0);
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
				QueueKeyEvent(SDL_KEYDOWN, sdlk, wchars[i]);
		}
		// dead-char; do nothing
		else if(output_count == -1)
		{
		}
		// translation failed; just generate a regular (non-unicode) event
		else if(output_count == 0)
			QueueKeyEvent(SDL_KEYDOWN, sdlk, 0);
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
// joystick

int SDL_NumJoysticks()
{
	return 0;
}

int SDL_JoystickEventState(int UNUSED(state))
{
	return 0;
}

const char* SDL_JoystickName(int UNUSED(device_index))
{
	return NULL;
}

SDL_Joystick* SDL_JoystickOpen(int UNUSED(device_index))
{
	return NULL;
}

int SDL_JoystickNumAxes(SDL_Joystick* UNUSED(joystick))
{
	return 0;
}

Sint16 SDL_JoystickGetAxis(SDL_Joystick* UNUSED(joystick), int UNUSED(axis))
{
	return 0;
}


//----------------------------------------------------------------------------
// app activation

enum SdlActivationType { LOSE = 0, GAIN = 1 };

static inline void QueueActiveEvent(SdlActivationType type, size_t changed_app_state)
{
	// SDL says this event is not generated when the window is created,
	// but skipping the first event may confuse things.

	SDL_Event ev;
	ev.type = SDL_ACTIVEEVENT;
	ev.active.state = (u8)changed_app_state;
	ev.active.gain  = (u8)((type == GAIN)? 1 : 0);
	QueueEvent(ev);
}


// SDL_APP* bitflags indicating whether we are active.
// note: responsibility for yielding lies with SDL apps -
// they control the main loop.
static Uint8 app_state;

static void active_ChangeState(SdlActivationType type, Uint8 changed_app_state)
{
	Uint8 old_app_state = app_state;

	if(type == GAIN)
		app_state = Uint8(app_state | changed_app_state);
	else
		app_state = Uint8(app_state & ~changed_app_state);

	// generate an event - but only if the given state flags actually changed.
	if((old_app_state & changed_app_state) != (app_state & changed_app_state))
		QueueActiveEvent(type, changed_app_state);
}


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
		{
			ShowWindow(g_hWnd, SW_RESTORE);
			ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
		}

		// re-assert mouse grab state
		SDL_WM_GrabInput(SDL_WM_GrabInput(SDL_GRAB_QUERY));
	}
	// deactivated (Alt+Tab out) or minimized
	else
	{
		type = LOSE;
		changed_app_state = SDL_APPINPUTFOCUS;
		if(fMinimized)
			changed_app_state |= SDL_APPACTIVE;

		key_ResetAll();

		gammaRamp.RestoreOriginal();
		if(fullscreen)
		{
			ChangeDisplaySettings(0, 0);
			ShowWindow(g_hWnd, SW_MINIMIZE);
		}
	}

	active_ChangeState(type, changed_app_state);
	return 0;
}


Uint8 SDL_GetAppState()
{
	return app_state;
}


static void QueueQuitEvent()
{
	SDL_Event ev;
	ev.type = SDL_QUIT;
	QueueEvent(ev);
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
//   windowDimensions-1. they are returned by mouse_GetCoords and have no prefix.

static void QueueMouseEvent(int x, int y)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEMOTION;
	debug_assert(unsigned(x|y) <= USHRT_MAX);
	ev.motion.x = (Uint16)x;
	ev.motion.y = (Uint16)y;
	QueueEvent(ev);
}

static void QueueButtonEvent(int button, int state, int x, int y)
{
	SDL_Event ev;
	ev.type = Uint8((state == SDL_PRESSED)? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP);
	ev.button.button = (u8)button;
	ev.button.state  = (u8)state;
	debug_assert(unsigned(x|y) <= USHRT_MAX);
	ev.button.x = (Uint16)x;
	ev.button.y = (Uint16)y;
	QueueEvent(ev);
}


static int mouse_x, mouse_y;

// generate a mouse move message and update our notion of the mouse position.
// x, y are client pixel coordinates.
// notes:
// - does not actually move the OS cursor;
// - called from mouse_Update and SDL_WarpMouse.
static void mouse_UpdatePosition(int x, int y)
{
	// nothing to do if it hasn't changed since last time
	if(mouse_x == x && mouse_y == y)
		return;

	mouse_x = x;
	mouse_y = y;
	QueueMouseEvent(x, y);
}

static POINT mouse_ScreenFromClient(int client_x, int client_y)
{
	POINT screen_pt;
	screen_pt.x = (LONG)client_x;
	screen_pt.y = (LONG)client_y;
	WARN_IF_FALSE(ClientToScreen(g_hWnd, &screen_pt));
	return screen_pt;
}

// get idealized client coordinates or return false if outside our window.
static bool mouse_GetCoords(int screen_x, int screen_y, int& x, int& y)
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


static void mouse_Update()
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
	if(mouse_GetCoords(screen_pt.x, screen_pt.y, x, y))
	{
		active_ChangeState(GAIN, SDL_APPMOUSEFOCUS);
		mouse_UpdatePosition(x, y);
	}
	// moved outside of window
	else
		active_ChangeState(LOSE, SDL_APPMOUSEFOCUS);
}


static unsigned mouse_buttons;

// (we define a new function signature since the windowsx.h message crackers
// don't provide for passing uMsg)
static LRESULT OnMouseButton(HWND UNUSED(hWnd), UINT uMsg, int client_x, int client_y, UINT flags)
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
	case WM_XBUTTONDOWN:
		button = SDL_BUTTON_X1 + GET_XBUTTON_WPARAM(flags) - 1;
		state = SDL_PRESSED;
		break;
	case WM_XBUTTONUP:
		button = SDL_BUTTON_X1 + GET_XBUTTON_WPARAM(flags) - 1;
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

	const POINT screen_pt = mouse_ScreenFromClient(client_x, client_y);
	int x, y;
	if(mouse_GetCoords(screen_pt.x, screen_pt.y, x, y))
		QueueButtonEvent(button, state, x, y);

	// Per MSDN, return TRUE for XBUTTON events
	if (uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONUP)
		return TRUE;

	return 0;
}


// (note: this message is sent even if the cursor is outside our window)
static LRESULT OnMouseWheel(HWND UNUSED(hWnd), int screen_x, int screen_y, int zDelta, UINT UNUSED(fwKeys))
{
	int x, y;
	if(mouse_GetCoords(screen_x, screen_y, x, y))
	{
		int button = (zDelta < 0)? SDL_BUTTON_WHEELDOWN : SDL_BUTTON_WHEELUP;
		// SDL says this sends a down message followed by up.
		QueueButtonEvent(button, SDL_PRESSED,  x, y);
		QueueButtonEvent(button, SDL_RELEASED, x, y);
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


void SDL_WarpMouse(int x, int y)
{
	// SDL interface provides for int, but the values should be
	// idealized client coords (>= 0)
	debug_assert(x >= 0 && y >= 0);
	mouse_UpdatePosition(x, y);

	const int client_x = x, client_y = y;
	const POINT screen_pt = mouse_ScreenFromClient(client_x, client_y);
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


SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode)
{
	static SDL_GrabMode prevMode;
	if(mode == SDL_GRAB_QUERY)
		return prevMode;
	prevMode = mode;

	if(mode == SDL_GRAB_OFF)
		ClipCursor(0);
	else
	{
		RECT clientRect;
		WARN_IF_FALSE(GetClientRect(g_hWnd, &clientRect));
		POINT upperLeft = { clientRect.left, clientRect.top };
		WARN_IF_FALSE(ClientToScreen(g_hWnd, &upperLeft));
		POINT lowerRight = { clientRect.right, clientRect.bottom };
		WARN_IF_FALSE(ClientToScreen(g_hWnd, &lowerRight));
		const RECT screenRect = { upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y };
		WARN_IF_FALSE(ClipCursor(&screenRect));
	}

	return mode;
}


//----------------------------------------------------------------------------
// video resizing/expose

static bool ResizeEventEnabled(int clientWidth, int clientHeight)
{
	// when the app receives a resize event, it must call SDL_SetVideoMode.
	// avoid that if the size hasn't actually changed. this also
	// prevents infinite recursion, since SDL_SetVideoMode might
	// trigger resize events. note that this logic is safer than a
	// skipResize flag, because that would remain set if SDL_SetVideoMode
	// doesn't trigger a resize, and we would miss an actual resize.
	static int lastClientWidth, lastClientHeight;
	if(lastClientWidth == clientWidth && lastClientHeight == clientHeight)
		return false;
	lastClientWidth = clientWidth;
	lastClientHeight = clientHeight;

	// if fullscreen, interaction with other topmost windows causes
	// minimization and a spurious resize. however, the app only
	// expects resizing events if !fullscreen.
	if(fullscreen)
		return false;

	// this happens during minimization, which results in an
	// app-active event anyway, and the app might have
	// trouble with size=0.
	if(clientWidth == 0 && clientHeight == 0)
		return false;

	return true;
}

// note: this is called continuously during resizing. since SDL doesn't
// discard any SDL_VIDEORESIZE events, the application must deal with
// the flood (and only call SDL_SetVideoMode once a frame or similar).
// note: SDL uses WM_WINDOWPOSCHANGING, which requires calling
// GetClientRect and suffers from false alarms.
static void OnSize(HWND UNUSED(hWnd), UINT UNUSED(state), int clientWidth, int clientHeight)
{
	if(!ResizeEventEnabled(clientWidth, clientHeight))
		return;

	SDL_Event ev;
	ev.type = SDL_VIDEORESIZE;
	ev.resize.w = clientWidth;
	ev.resize.h = clientHeight;
	QueueEvent(ev);
}


static BOOL OnEraseBkgnd(HWND UNUSED(hWnd), HDC UNUSED(hDC))
{
	SDL_Event ev;
	ev.type = SDL_VIDEOEXPOSE;
	QueueEvent(ev);

	// prevent GDI from erasing the background by claiming we did so.
	// PAINTSTRUCT.fErase will later be FALSE.
	return TRUE;
}


//----------------------------------------------------------------------------

static LRESULT OnPaint(HWND hWnd)
{
	// BeginPaint/EndPaint is unnecessary (see http://opengl.czweb.org/ch04/082-084.html)
	// however, we at least need to validate the window to prevent
	// continuous WM_PAINT messages.
	ValidateRect(hWnd, 0);
	return 0;
}

static LRESULT OnDestroy(HWND hWnd)
{
	debug_assert(hWnd == g_hWnd);
	WARN_IF_FALSE(ReleaseDC(g_hWnd, g_hDC));
	g_hDC = (HDC)INVALID_HANDLE_VALUE;
	g_hWnd = (HWND)INVALID_HANDLE_VALUE;
	QueueQuitEvent();

#ifdef _DEBUG
	// see http://www.adrianmccarthy.com/blog/?p=51
	// with WM_QUIT in the message queue, MessageBox will immediately
	// return IDABORT. to ensure any subsequent CRT error reports are
	// at least somewhat visible, we redirect them to debug output.
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif

	PostQuitMessage(0);
	return 0;
}


enum DefWindowProcDisposition
{
	kSkipDefWindowProc,
	kRunDefWindowProc
};

static DefWindowProcDisposition OnSysCommand(WPARAM wParam)
{
	switch(wParam)
	{
	case SC_SCREENSAVE:
		// disable screen-saver in fullscreen mode (other applications
		// may interfere with us, and have set the system-wide gamma)
		if(fullscreen)
			return kSkipDefWindowProc;
		break;

	// Alt+F4 or system menu doubleclick/exit
	case SC_CLOSE:
		QueueQuitEvent();
		break;
	}

	return kRunDefWindowProc;
}


static LRESULT CALLBACK OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(is_quitting)
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);

	switch(uMsg)
	{
	case WM_PAINT:
		return OnPaint(hWnd);

	HANDLE_MSG(hWnd, WM_ERASEBKGND, OnEraseBkgnd);

	// prevent selecting menu in fullscreen mode
	case WM_NCHITTEST:
		if(fullscreen)
			return HTCLIENT;
		break;

	HANDLE_MSG(hWnd, WM_ACTIVATE, OnActivate);
	HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);

	case WM_SYSCOMMAND:
		if(OnSysCommand(wParam) == kSkipDefWindowProc)
			return 0;
		break;

	HANDLE_MSG(hWnd, WM_SYSKEYUP  , OnKey);
	HANDLE_MSG(hWnd, WM_KEYUP     , OnKey);
	HANDLE_MSG(hWnd, WM_SYSKEYDOWN, OnKey);
	HANDLE_MSG(hWnd, WM_KEYDOWN   , OnKey);

	HANDLE_MSG(hWnd, WM_MOUSEWHEEL, OnMouseWheel);

	HANDLE_MSG(hWnd, WM_SIZE, OnSize);

	// (can't use message crackers: they do not provide for passing uMsg)
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		return OnMouseButton(hWnd, uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT)wParam);

	default:
		// (DefWindowProc must be called outside the switch because some
		// messages are only conditionally 'grabbed', e.g. NCHITTEST)
		break;
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}


void SDL_PumpEvents()
{
	// rationale: we would like to reduce CPU usage automatically if
	// possible. blocking here until a message arrives would accomplish
	// that, but might potentially freeze the app too long.
	// instead, they should check active state and call SDL_Delay etc.
	// if our window is minimized.

	mouse_Update();	// polled

	MSG msg;
	while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
	{
		DispatchMessageW(&msg);
	}
}



int SDL_PollEvent(SDL_Event* ev)
{
	SDL_PumpEvents();

	if(DequeueEvent(*ev))
		return 1;

	return 0;
}


int SDL_PushEvent(SDL_Event* ev)
{
	QueueEvent(*ev);
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
	HANDLE h = CreateSemaphore(0, cnt, std::numeric_limits<LONG>::max(), 0);
	return sem_from_HANDLE(h);
}

void SDL_DestroySemaphore(SDL_sem* sem)
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


//-----------------------------------------------------------------------------
// misc API

void SDL_WM_SetCaption(const char* title, const char* icon)
{
	WARN_IF_FALSE(SetWindowTextA(g_hWnd, title));

	// real SDL ignores this parameter, so we will follow suit.
	UNUSED2(icon);
}


u32 SDL_GetTicks()
{
	return GetTickCount();
}


void SDL_Delay(Uint32 ms)
{
	Sleep(ms);
}


void* SDL_GL_GetProcAddress(const char* name)
{
	return wglGetProcAddress(name);
}


//-----------------------------------------------------------------------------
// init/shutdown

// defend against calling SDL_Quit twice (GameSetup does this to work
// around ATI driver breakage)
static ModuleInitState initState;

static LibError Init()
{
	key_Init();
	return INFO::OK;
}

static void Shutdown()
{
	is_quitting = true;

	if(wutil_IsValidHandle(g_hDC))
		gammaRamp.RestoreOriginal();

	if(wutil_IsValidHandle(g_hWnd))
		WARN_IF_FALSE(DestroyWindow(g_hWnd));

	video_Shutdown();
}

int SDL_Init(Uint32 UNUSED(flags))
{
	return (ModuleInit(&initState, Init) < 0)? -1 : 0;
}

int SDL_InitSubSystem(Uint32 UNUSED(flags))
{
	return 0;
}

void SDL_Quit()
{
	ModuleShutdown(&initState, Shutdown);
}


static NativePath GetStdoutPathname()
{
	// the current directory is unreliable, so use the full path to
	// the current executable.
	wchar_t pathnameEXE[MAX_PATH];
	const DWORD charsWritten = GetModuleFileNameW(0, pathnameEXE, ARRAY_SIZE(pathnameEXE));
	debug_assert(charsWritten);
	const NativePath path = Path::Path(pathnameEXE);

	// add the EXE name to the filename to allow multiple executables
	// with their own redirections. (we can't use wutil_ExecutablePath
	// because it doesn't return the basename)
	NativePath name = Path::Basename(pathnameEXE);
	NativePath pathname = Path::Join(path, (name+L"_stdout.txt"));

	return pathname;
}

static void RedirectStdout()
{
	// this process is apparently attached to a console, and users might be
	// surprised to find that we redirected the output to a file, so don't.
	if(wutil_IsValidHandle(GetStdHandle(STD_OUTPUT_HANDLE)))
		return;

	const NativePath pathname = GetStdoutPathname();

 	// ignore BoundsChecker warnings here. subsystem is set to "Windows"
 	// to prevent the OS from opening a console on startup (ugly).
	// that means stdout isn't associated with a lowio handle; _close is
 	// called with fd = -1. oh well, there's nothing we can do.
 	FILE* f = 0;
	// (return value ignored - it indicates 'file already exists' even
	// if f is valid)
	(void)_wfreopen_s(&f, pathname.c_str(), L"wt", stdout);
	// executable directory (probably Program Files) is read-only for
	// non-Administrators. we can't pick another directory because
	// ah_log_dir might not be valid until the app's init has run,
	// nor should we pollute the (root) wutil_AppdataPath directory.
	// since stdout usually isn't critical and is seen if launching the
	// app from a console, just skip the redirection in this case.
	if(f == 0)
		return;

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
