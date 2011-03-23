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

#include "precompiled.h"

#include "lib/lib.h"
#include "lib/utf8.h"

#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/gfx.h"

#include <mach-o/dyld.h>


// "copy" text into the clipboard. replaces previous contents.
LibError sys_clipboard_set(const wchar_t* text)
{
	return INFO::OK;
}

// allow "pasting" from clipboard. returns the current contents if they
// can be represented as text, otherwise 0.
// when it is no longer needed, the returned pointer must be freed via
// sys_clipboard_free. (NB: not necessary if zero, but doesn't hurt)
wchar_t* sys_clipboard_get(void)
{
	// Remember to implement sys_clipboard_free when implementing this method!
	return NULL;
}

// frees memory used by <copy>, which must have been returned by
// sys_clipboard_get. see note above.
LibError sys_clipboard_free(wchar_t* copy)
{
	// Since clipboard_get never returns allocated memory (unimplemented), we
	// should only ever get called with a NULL pointer.
	debug_assert(!copy);
	return INFO::OK;
}


/**
 * get current video mode.
 *
 * this is useful when choosing a new video mode.
 *
 * @param xres, yres (optional out) resolution [pixels]
 * @param bpp (optional out) bits per pixel
 * @param freq (optional out) vertical refresh rate [Hz]
 * @return LibError; INFO::OK unless: some information was requested
 * (i.e. pointer is non-NULL) but cannot be returned.
 * on failure, the outputs are all left unchanged (they are
 * assumed initialized to defaults)
 **/
LibError gfx_get_video_mode(int* xres, int* yres, int* bpp, int* freq)
{
	// TODO Implement
	return ERR::NOT_IMPLEMENTED;
}


OsPath sys_ExecutablePathname()
{
	static char name[PATH_MAX];
	static bool init = false;
	if ( !init )
	{
		init = true;
		char temp[PATH_MAX];
		u32 size = PATH_MAX;
		if (_NSGetExecutablePath( temp, &size ))
			return OsPath();
		realpath(temp, name);
	}
	
	// On OS X, we might be in a bundle. In this case set its name as our name.
	char* app = strstr(name, ".app");
	if (app) {
		// Remove everything after the .app
		*(app + strlen(".app")) = '\0';
		debug_printf(L"app bundle name: %hs\n", name);
	}
	
	return name;
}
