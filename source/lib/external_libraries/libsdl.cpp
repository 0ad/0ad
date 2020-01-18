/* Copyright (C) 2020 Wildfire Games.
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

#include "precompiled.h"

#include "libsdl.h"

#include "lib/debug.h"

#include <SDL_syswm.h>

const char* GetSDLSubsystem(SDL_Window* window)
{
	SDL_SysWMinfo wminfo;
	// The info structure must be initialized with the SDL version.
	SDL_VERSION(&wminfo.version);

	if (!SDL_GetWindowWMInfo(window, &wminfo))
	{
		debug_printf("Failed to query SDL WM info: %s", SDL_GetError());
		return nullptr;
	}

	const char* subsystem = "unknown";
	switch (wminfo.subsystem)
	{
	case SDL_SYSWM_WAYLAND:
		subsystem = "Wayland";
		break;
	case SDL_SYSWM_X11:
		subsystem = "X11";
		break;
	case SDL_SYSWM_WINDOWS:
		subsystem = "Windows";
		break;
	case SDL_SYSWM_COCOA:
		subsystem = "Cocoa";
		break;
	case SDL_SYSWM_UIKIT:
		subsystem = "UIKit";
		break;
	case SDL_SYSWM_DIRECTFB:
		subsystem = "DirectFB";
		break;
	case SDL_SYSWM_MIR:
		subsystem = "Mir";
		break;
#if SDL_VERSION_ATLEAST(2, 0, 3)
	case SDL_SYSWM_WINRT:
		subsystem = "WinRT";
		break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 4)
	case SDL_SYSWM_ANDROID:
		subsystem = "Android";
		break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 5)
	case SDL_SYSWM_VIVANTE:
		subsystem = "Vivante";
		break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 6)
	case SDL_SYSWM_OS2:
		subsystem = "OS/2";
		break;
#endif
	// Insert newer supported platforms here.
#if SDL_VERSION_ATLEAST(2, 0, 11)
	default:
		debug_printf("Unknown platform, please add it to source/lib/external_libraries/libsdl.cpp\n");
#endif
	case SDL_SYSWM_UNKNOWN:
		break;
	}

	return subsystem;
}
