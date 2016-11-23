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

#include "precompiled.h"

#include "lib/lib.h"
#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/gfx.h"
#include "lib/utf8.h"
#include "osx_bundle.h"
#include "osx_pasteboard.h"

#include <ApplicationServices/ApplicationServices.h>
#include <AvailabilityMacros.h> // MAC_OS_X_VERSION_MIN_REQUIRED
#include <CoreFoundation/CoreFoundation.h>
#include <mach-o/dyld.h> // _NSGetExecutablePath

// Ignore deprecation warnings for 10.5 backwards compatibility
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

Status sys_clipboard_set(const wchar_t* text)
{
	Status ret = INFO::OK;

	std::string str = utf8_from_wstring(text);
	bool ok = osx_SendStringToPasteboard(str);
	if (!ok)
		ret = ERR::FAIL;
	return ret;
}

wchar_t* sys_clipboard_get()
{
	wchar_t* ret = NULL;
	std::string str;
	bool ok = osx_GetStringFromPasteboard(str);
	if (ok)
	{
		// TODO: this is yucky, why are we passing around wchar_t*?
		std::wstring wstr = wstring_from_utf8(str);
		size_t len = wcslen(wstr.c_str());
		ret = (wchar_t*)malloc((len+1)*sizeof(wchar_t));
		std::copy(wstr.c_str(), wstr.c_str()+len, ret);
		ret[len] = 0;
	}

	return ret;
}

Status sys_clipboard_free(wchar_t* copy)
{
	free(copy);
	return INFO::OK;
}


namespace gfx {

Status GetVideoMode(int* xres, int* yres, int* bpp, int* freq)
{
	if(xres)
		*xres = (int)CGDisplayPixelsWide(kCGDirectMainDisplay);

	if(yres)
		*yres = (int)CGDisplayPixelsHigh(kCGDirectMainDisplay);

	if(bpp)
	{
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
		// CGDisplayBitsPerPixel was deprecated in OS X 10.6
		if (CGDisplayCopyDisplayMode != NULL)
		{
			CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);
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
			CGDisplayModeRelease(currentMode);
		}
		else
		{
#endif		// fallback to 10.5 API
			CFDictionaryRef currentMode = CGDisplayCurrentMode(kCGDirectMainDisplay);
			CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(currentMode, kCGDisplayBitsPerPixel);
			CFNumberGetValue(num, kCFNumberIntType, bpp);
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
		}
#endif
	}

	if(freq)
	{
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
		if (CGDisplayCopyDisplayMode != NULL)
		{
			CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);
			*freq = (int)CGDisplayModeGetRefreshRate(currentMode);
			
			// We're responsible for this
			CGDisplayModeRelease(currentMode);
		}
		else
		{
#endif		// fallback to 10.5 API
			CFDictionaryRef currentMode = CGDisplayCurrentMode(kCGDirectMainDisplay);
			CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(currentMode, kCGDisplayRefreshRate);
			CFNumberGetValue(num, kCFNumberIntType, freq);
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
		}
#endif
	}

	return INFO::OK;
}

Status GetMonitorSize(int& width_mm, int& height_mm)
{
	CGSize screenSize = CGDisplayScreenSize(kCGDirectMainDisplay);

	if (screenSize.width == 0 || screenSize.height == 0)
		return ERR::FAIL;

	width_mm = screenSize.width;
	height_mm = screenSize.height;

	return INFO::OK;
}

}	// namespace gfx


OsPath sys_ExecutablePathname()
{
	OsPath path;

	// On OS X we might be a bundle, return the bundle path as the executable name,
	//	i.e. /path/to/0ad.app instead of /path/to/0ad.app/Contents/MacOS/pyrogenesis
	if (osx_IsAppBundleValid())
	{
		path = osx_GetBundlePath();
		ENSURE(!path.empty());
	}
	else
	{
		char temp[PATH_MAX];
		u32 size = PATH_MAX;
		if (_NSGetExecutablePath(temp, &size) == 0)
		{
			char name[PATH_MAX];
			realpath(temp, name);
			path = OsPath(name);
		}
	}

	return path;
}

#pragma GCC diagnostic pop // restore user flags
