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

#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/gfx.h"

#include <mach-o/dyld.h>
#include <ApplicationServices/ApplicationServices.h>


// "copy" text into the clipboard. replaces previous contents.
Status sys_clipboard_set(const wchar_t* text)
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
Status sys_clipboard_free(wchar_t* copy)
{
	// Since clipboard_get never returns allocated memory (unimplemented), we
	// should only ever get called with a NULL pointer.
	ENSURE(!copy);
	return INFO::OK;
}


namespace gfx {

Status GetVideoMode(int* xres, int* yres, int* bpp, int* freq)
{
	// TODO: This breaks 10.5 compatibility, as CGDisplayCopyDisplayMode
	//  and CGDisplayModeCopyPixelEncoding were not available
	CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);

	if(xres)
		*xres = (int)CGDisplayPixelsWide(kCGDirectMainDisplay);

	if(yres)
		*yres = (int)CGDisplayPixelsHigh(kCGDirectMainDisplay);

	if(bpp)
	{
		// CGDisplayBitsPerPixel was deprecated in OS X 10.6
		CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(currentMode);
		if (CFStringCompare(pixelEncoding, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
			*bpp = 32;
		else if (CFStringCompare(pixelEncoding, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
			*bpp = 16;
		else if (CFStringCompare(pixelEncoding, CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
			*bpp = 8;
		else	// error
			*bpp = 0;

		// We're responsible for this
		CFRelease(pixelEncoding);
	}

	if(freq)
		*freq = (int)CGDisplayModeGetRefreshRate(currentMode);

	// We're responsible for this
	CGDisplayModeRelease(currentMode);

	return INFO::OK;
}

Status GetMonitorSize(int* xres, int* yres, int* bpp, int* freq)
{
	// TODO Implement
	return ERR::NOT_SUPPORTED;	// NOWARN
}

}	// namespace gfx


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
