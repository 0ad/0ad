/* Copyright (C) 2013 Wildfire Games.
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

#import <AvailabilityMacros.h> // MAC_OS_X_VERSION_MIN_REQUIRED
#import <Foundation/Foundation.h>
#import <string>

#import "osx_bundle.h"

bool osx_IsAppBundleValid()
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	NSBundle *bundle = [NSBundle mainBundle];
	// mainBundle can create an NSBundle even with a loose executable.
	// Assume that if the identifier is defined, we are actually inside a bundle.
	NSString *identifier = [bundle bundleIdentifier];

	[pool drain];
	return bundle != nil && identifier != nil;
}

namespace {
std::string GetBundlePath(SEL selector)
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	std::string path;

	NSBundle *bundle = [NSBundle mainBundle];
	if (bundle != nil)
	{
		NSString *pathStr;
		// Retrieve NSURL and convert to POSIX path, then get C-string
		//	encoded as UTF-8, and use it to construct std::string
		// NSURL:path "If the receiver does not conform to RFC 1808, returns nil."
		pathStr = [[bundle performSelector:selector] path];

		if (pathStr != nil)
			path = std::string([pathStr UTF8String]);
	}

	[pool drain];
	return path;
}
}

std::string osx_GetBundlePath()
{
	return GetBundlePath(@selector(bundleURL));
}

std::string osx_GetBundleResourcesPath()
{
	return GetBundlePath(@selector(resourceURL));
}

std::string osx_GetBundleFrameworksPath()
{
	return GetBundlePath(@selector(privateFrameworksURL));
}
