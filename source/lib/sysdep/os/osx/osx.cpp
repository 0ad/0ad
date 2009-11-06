/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "lib/lib.h"
#include "lib/wchar.h"

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


LibError sys_get_executable_name(fs::wpath& pathname)
{
	static char name[PATH_MAX];
	static bool init = false;
	if ( !init )
	{
		init = true;
		char temp[PATH_MAX];
		u32 size = PATH_MAX;
		if (_NSGetExecutablePath( temp, &size ))
		{
			return ERR::NO_SYS;
		}
		debug_printf(L"exe name before realpath: %hs\n", temp);
		realpath(temp, name);
		debug_printf(L"exe name after realpath: %hs\n", temp);
	}
	
	// On OS X, we might be in a bundle. In this case set its name as our name.
	char* app = strstr(name, ".app");
	if (app) {
		// Remove everything after the .app
		*(app + strlen(".app")) = '\0';
		debug_printf(L"app bundle name: %hs\n", name);
	}
	
	pathname = wstring_from_string(name);
	debug_printf(L"returning exe name: %hs\n", name);
	
	return INFO::OK;
}
