/* Copyright (C) 2021 Wildfire Games.
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
#include "lib/utf8.h"
#include "osx_bundle.h"

#include <ApplicationServices/ApplicationServices.h>
#include <AvailabilityMacros.h> // MAC_OS_X_VERSION_MIN_REQUIRED
#include <CoreFoundation/CoreFoundation.h>
#include <mach-o/dyld.h> // _NSGetExecutablePath

// Ignore deprecation warnings for 10.5 backwards compatibility
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

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
