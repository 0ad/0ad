#include "precompiled.h"

#include "lib/lib.h"

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


LibError sys_get_executable_name(char* n_path, size_t buf_size)
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
		debug_printf("exe name before realpath: %s\n", temp);
		realpath(temp, name);
		debug_printf("exe name after realpath: %s\n", temp);
	}
	
	// On OS X, we might be in a bundle. In this case set its name as our name.
	char* app = strstr(name, ".app");
	if (app) {
		// Remove everything after the .app
		*(app + strlen(".app")) = '\0';
		debug_printf("app bundle name: %s\n", name);
	}
	
	strncpy(n_path, name, buf_size);
	debug_printf("returning exe name: %s\n", name);
	
	return INFO::OK;
}
