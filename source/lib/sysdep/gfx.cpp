/* Copyright (c) 2015 Wildfire Games
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
 * graphics card detection.
 */

#include "precompiled.h"
#include "lib/sysdep/gfx.h"

#include "lib/external_libraries/libsdl.h"
#include "lib/ogl.h"

#if OS_WIN
# include "lib/sysdep/os/win/wgfx.h"
#endif


namespace gfx {

std::wstring CardName()
{
	// GL_VENDOR+GL_RENDERER are good enough here, so we don't use WMI to detect the cards.
	// On top of that WMI can cause crashes with Nvidia Optimus and some netbooks
	// see http://trac.wildfiregames.com/ticket/1952
	//     http://trac.wildfiregames.com/ticket/1575
	wchar_t cardName[128];
	const char* vendor   = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	// (happens if called before ogl_Init or between glBegin and glEnd.)
	if(!vendor || !renderer)
		return L"";
	swprintf_s(cardName, ARRAY_SIZE(cardName), L"%hs %hs", vendor, renderer);

	// remove crap from vendor names. (don't dare touch the model name -
	// it's too risky, there are too many different strings)
#define SHORTEN(what, charsToKeep)\
	if(!wcsncmp(cardName, what, ARRAY_SIZE(what)-1))\
		memmove(cardName+charsToKeep, cardName+ARRAY_SIZE(what)-1, (wcslen(cardName)-(ARRAY_SIZE(what)-1)+1)*sizeof(wchar_t));
	SHORTEN(L"ATI Technologies Inc.", 3);
	SHORTEN(L"NVIDIA Corporation", 6);
	SHORTEN(L"S3 Graphics", 2);					// returned by EnumDisplayDevices
	SHORTEN(L"S3 Graphics, Incorporated", 2);	// returned by GL_VENDOR
#undef SHORTEN

	return cardName;
}


std::wstring DriverInfo()
{
	std::wstring driverInfo;
#if OS_WIN
	driverInfo = wgfx_DriverInfo();
	if(driverInfo.empty())
#endif
	{
		const char* version = (const char*)glGetString(GL_VERSION);
		if(version)
		{
			// add "OpenGL" to differentiate this from the real driver version
			// (returned by platform-specific detect routines).
			driverInfo = std::wstring(L"OpenGL ") + std::wstring(version, version+strlen(version));
		}
	}

	if(driverInfo.empty())
		return L"(unknown)";
	return driverInfo;
}


size_t MemorySizeMiB()
{
	// TODO: not implemented, SDL_GetVideoInfo only works on some platforms in SDL 1.2
	//	and no replacement is available in SDL2, and it can crash with Nvidia Optimus
	//	see http://trac.wildfiregames.com/ticket/2145
	debug_warn(L"MemorySizeMiB not implemented");
	return 0;
}

}	// namespace gfx
