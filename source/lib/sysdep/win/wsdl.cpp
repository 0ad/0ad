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

 
// TODO: should use GetMessage when not active to reduce CPU load.
// where to do this?
// - force the app to check for SDL's activation messages, and call
//   sdl-wait-message?
// - do it here, just make SDL_PollEvent block until message received?
// - have the app use another free-the-cpu method, since it controls the main loop.
//   this is what's currently happening.

#include "precompiled.h"

#include <process.h>

#include "sdl.h"
#include "lib.h"
#include "posix.h"
#include "win_internal.h"

#include "ogl.h"	// needed to pull in the delay-loaded opengl32.dll

#include "SDL_vkeys.h"
/*/*#include "ps/Hotkey.h"*/


#include <ddraw.h>
#include <process.h>	// _beginthreadex

#include <stdio.h>
#include <math.h>

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


#pragma data_seg(WIN_CALLBACK_PRE_MAIN(b))
WIN_REGISTER_FUNC(wsdl_init);
#pragma data_seg(WIN_CALLBACK_POST_ATEXIT(d))
WIN_REGISTER_FUNC(wsdl_shutdown);
#pragma data_seg()


// SDL_APP* bitflags indicating whether we are active.
// note: responsibility for yielding lies with SDL apps -
// they control the main loop.
static uint app_state;

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
											

static DEVMODE dm;			// current video mode
static HDC hDC = (HDC)INVALID_HANDLE_VALUE;
static HGLRC hGLRC = (HGLRC)INVALID_HANDLE_VALUE;

static int depth_bits = 24;	// depth buffer size; set via SDL_GL_SetAttribute


Uint8 SDL_GetAppState()
{
	return app_state;
}


//----------------------------------------------------------------------------

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

static inline void queue_active_event(uint gain, uint changed_app_state)
{
	// SDL says this event is not generated when the window is created,
	// but skipping the first event may confuse things.

	SDL_Event ev;
	ev.type = SDL_ACTIVEEVENT;
	ev.active.state = (u8)changed_app_state;
	ev.active.gain = (u8)gain;
	queue_event(ev);
}

static void queue_quit_event()
{
	SDL_Event ev;
	ev.type = SDL_QUIT;
	queue_event(ev);
}

static void queue_button_event(uint button, uint state, uint x, uint y)
{
	SDL_Event ev;
	ev.type = (state == SDL_PRESSED)? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
	ev.button.button = (u8)button;
	ev.button.state  = (u8)state;
	ev.button.x = x;
	ev.button.y = y;
	queue_event(ev);
}


//----------------------------------------------------------------------------
// gamma
//----------------------------------------------------------------------------

static bool gamma_changed;
static u16 org_ramp[3][256];
static u16 cur_ramp[3][256];


// ramp: 8.8 fixed point
static int calc_gamma_ramp(float gamma, u16* ramp)
{
	// assume identity if invalid
	if(gamma <= 0.0f)
		gamma = 1.0f;

	// identity: special-case to make sure we get exact values
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
		// don't add 1/256 - this isn't time-critical and
		// accuracy is more important.
		// need a temp variable to disambiguate pow() argument type.
		ramp[i] = fp_to_u16(pow(frac, inv_gamma));
	}

	return 0;
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

	CHECK_ERR(calc_gamma_ramp(r, cur_ramp[0]));
	CHECK_ERR(calc_gamma_ramp(g, cur_ramp[1]));
	CHECK_ERR(calc_gamma_ramp(b, cur_ramp[2]));

	if(!SetDeviceGammaRamp(hDC, cur_ramp))
		return -1;

	gamma_changed = true;
	return 0;
}


//----------------------------------------------------------------------------

static Uint8 keys[SDLK_LAST];

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


Uint8* SDL_GetKeyState(int* num_keys)
{
	if(num_keys)
		*num_keys = SDLK_LAST;
	return keys;
}


//----------------------------------------------------------------------------

static u16 mouse_x, mouse_y;

static uint mouse_buttons;

Uint8 SDL_GetMouseState(int* x, int* y)
{
	if(x)
		*x = (int)mouse_x;
	if(y)
		*y = (int)mouse_y;
	return (Uint8)mouse_buttons;
}


//----------------------------------------------------------------------------

static LRESULT CALLBACK wndproc(HWND hWnd, uint uMsg, WPARAM wParam, LPARAM lParam)
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

	case WM_ACTIVATE:
		{
		const uint wa_type = LOWORD(wParam);
		const bool is_minimized = (HIWORD(wParam) != 0);
		uint changed_app_state;
		uint gain;
		if(wa_type != WA_INACTIVE && !is_minimized)
		{
			gain = 1;
			changed_app_state = SDL_APPINPUTFOCUS|SDL_APPACTIVE;
			app_state |= changed_app_state;
		}
		// deactivated (Alt+Tab out) or minimized
		else
		{
			gain = 0;
			changed_app_state = SDL_APPINPUTFOCUS;
			if(is_minimized)
				changed_app_state |= SDL_APPACTIVE;
			app_state &= ~changed_app_state;
		}

		if(gain)
		{
			gamma_swap(GAMMA_LATCH_NEW_RAMP);
			if(fullscreen)
			{
				ShowWindow(hWnd, SW_RESTORE);
				ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
			}
		}
		else
		{
			reset_all_keys();

			gamma_swap(GAMMA_RESTORE_ORIGINAL);
			if(fullscreen)
			{
				ChangeDisplaySettings(0, 0);
				ShowWindow(hWnd, SW_MINIMIZE);
			}
		}

		queue_active_event(gain, changed_app_state);

		// let DefWindowProc run (gives us keyboard focus if
		// we're being reactivated)
		break;
		}

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

	case WM_SYSKEYUP:
	case WM_KEYUP:
		// TODO Mappings for left/right modifier keys
		// TODO Modifier statekeeping

		{
		uint sdlk = vkmap((int)wParam);
		if(sdlk != SDLK_UNKNOWN)
			keys[sdlk] = 0;

		SDL_Event ev;
		ev.type = SDL_KEYUP;
		ev.key.keysym.sym = (SDLKey)sdlk;
		ev.key.keysym.unicode = 0;
		queue_event(ev);
		}
		return 1;

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		{
		uint sdlk = vkmap((int)wParam);
		if(sdlk != SDLK_UNKNOWN)
			keys[sdlk] = 1;
		break;
		}

	case WM_MOUSEWHEEL:
		{
			short delta = (short)HIWORD(wParam);
			uint button = (delta < 0)? SDL_BUTTON_WHEELDOWN : SDL_BUTTON_WHEELUP;
			uint x = LOWORD(lParam);
			uint y = HIWORD(lParam);
			// SDL says this sends a down message followed by up.
			queue_button_event(button, SDL_PRESSED , x, y);
			queue_button_event(button, SDL_RELEASED, x, y);
		}
		break;


	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
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
		default:
			UNREACHABLE;
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
			// release after all up events received
			if(--outstanding_press_events <= 0)
			{
				ReleaseCapture();
				outstanding_press_events = 0;
			}

		// update bits
		if(state == SDL_PRESSED)
			mouse_buttons |= SDL_BUTTON(button);
		else
			mouse_buttons &= ~SDL_BUTTON(button);

		uint x = LOWORD(lParam);
		uint y = HIWORD(lParam);
		queue_button_event(button, state, x, y);
		break;
		}

	default:
		// can't call DefWindowProc here: some messages
		// are only conditionally 'grabbed' (e.g. NCHITTEST)
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


// always on (we don't care about the extra overhead)
int SDL_EnableUNICODE(int UNUSED(enable))
{
	return 1;
}

// Translate Windows VK's into our SDLK's for keys that must be translated
int SDL_WaitEvent(SDL_Event* ev)
{
	debug_assert(ev == 0 && "storing ev isn't implemented");
	WaitMessage();
	return 0;
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
	debug_assert(num_chars <= CHAR_BUF_SIZE);
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


	if(dequeue_event(ev))
		return 1;

	// events that trigger messages (mouse done below)
	MSG msg;
	while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
	{
		DispatchMessageW(&msg);

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

				//printf("ToUnicode: No translation for %02x, extended flag %d, scancode %d, SDLK %02x [%c]\n", vk, msg.lParam & 0x01000000, scancode, ev.key.keysym.sym, ev.key.keysym.sym);

				return 1;
			}
		}
	}

	// mouse motion
	//
	// don't use DirectInput, because we want to respect the user's mouse
	// sensitivity settings. Windows messages are laggy, so poll instead.
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(hWnd, &p);
	if(mouse_x != p.x || mouse_y != p.y)
	{
		ev->type = SDL_MOUSEMOTION;
		ev->motion.x = mouse_x = (u16)p.x;
		ev->motion.y = mouse_y = (u16)p.y;
		return 1;
	}

	return 0;
}

int SDL_PushEvent(SDL_Event* ev)
{
	queue_event(*ev);
	return 0;
}

int SDL_GL_SetAttribute(SDL_GLattr attr, int value)
{
	if(attr == SDL_GL_DEPTH_SIZE)
		depth_bits = value;

	return 0;
}


//
// keyboard hook - used to intercept PrintScreen and system keys (e.g. Alt+Tab)
//

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
			/*
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
			*/
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




static int wsdl_init()
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
	FILE* ret = freopen(path, "wt", stdout);
	if(!ret)
		debug_warn("stdout freopen failed");

#if CONFIG_PARANOIA
	// disable buffering, so that no writes are lost even if the program
	// crashes. only enabled in full debug mode because this is really slow!
	setvbuf(stdout, 0, _IONBF, 0);
#endif

	enable_kbd_hook(true);

	return 0;
}


static int wsdl_shutdown()
{
	is_shutdown = true;

	// redirected to stdout.txt in SDL_Init;
	// close to avoid BoundsChecker warning.
	fclose(stdout);

	gamma_swap(GAMMA_RESTORE_ORIGINAL);

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

	enable_kbd_hook(false);

	return 0;
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

	// check if resolution needs to be changed
	// .. invalid: keep current settings
	if(w <= 0 || h <= 0 || bpp <= 0)
		goto keep;
	// .. higher resolution mode needed
	if(w > cur_w || h > cur_h)
		goto change;
	// .. fullscreen requested and not exact same mode set
	if(fullscreen && (w != cur_w || h != cur_h))
	{
change:
		dm.dmPelsWidth  = (DWORD)w;
		dm.dmPelsHeight = (DWORD)h;
		dm.dmFields |= DM_PELSWIDTH|DM_PELSHEIGHT;
	}
keep:

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

	hWnd = CreateWindowEx(0, (LPCSTR)class_atom, APP_NAME, windowStyle, 0, 0, w, h, 0, 0, hInst, 0);
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


void SDL_GL_SwapBuffers()
{
	SwapBuffers(hDC);
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

	UNUSED2(icon);	// TODO: implement
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

inline u32 SDL_GetTicks()
{
	return GetTickCount();
}


inline void SDL_Delay(Uint32 ms)
{
	Sleep(ms);
}



inline int SDL_WarpMouse(int x, int y)
{
	return SetCursorPos(x, y);
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




void* SDL_GL_GetProcAddress(const char* name)
{
	return wglGetProcAddress(name);
}


//////////////////////////////////////////////////////////////////////////////


//
// semaphores
//

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


//
// threads
//

// users don't need to allocate SDL_Thread variables, so type = void
// API returns SDL_Thread*, which is the HANDLE value itself.
//
// we go through hoops to avoid type cast warnings;
// a simple union { pthread_t; SDL_Thread* } yields "uninitialized"
// warnings in VC2005, so we coerce values directly.
cassert(sizeof(pthread_t) == sizeof(SDL_Thread*));

SDL_Thread* SDL_CreateThread(int(*func)(void*), void* param)
{
	pthread_t thread = 0;
	if(pthread_create(&thread, 0, (void*(*)(void*))func, param) < 0)
		return 0;
	return *(SDL_Thread**)&thread;
}

int SDL_KillThread(SDL_Thread* thread)
{
	pthread_cancel(*(pthread_t*)&thread);
	return 0;
}

