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

#define STRINGIZE2(id) # id
#define STRINGIZE(id) STRINGIZE2(id)

// Pass the bundle identifier string as a build option
#ifdef BUNDLE_IDENTIFIER
static const char* BUNDLE_ID_STR = STRINGIZE(BUNDLE_IDENTIFIER);
#else
static const char* BUNDLE_ID_STR = "";
#endif


bool osx_IsAppBundleValid()
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	// Check for the existence of bundle with correct identifier property
	//  (can't just use mainBundle because that can return a bundle reference
	//  even for a loose binary!)
	NSBundle *bundle = [NSBundle bundleWithIdentifier: [NSString stringWithUTF8String: BUNDLE_ID_STR]];

	[pool drain];
	return bundle != nil;
}

std::string osx_GetBundlePath()
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	std::string path;

	NSBundle *bundle = [NSBundle bundleWithIdentifier: [NSString stringWithUTF8String: BUNDLE_ID_STR]];
	if (bundle != nil)
	{
		NSString *pathStr;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
		// Retrieve NSURL and convert to POSIX path, then get C-string
		//	encoded as UTF-8, and use it to construct std::string
		// NSURL:path "If the receiver does not conform to RFC 1808, returns nil."
		if ([bundle respondsToSelector: @selector(bundleURL)])
			pathStr = [[bundle bundleURL] path];
		else
#endif
		pathStr = [bundle bundlePath];

		if (pathStr != nil)
			path = std::string([pathStr UTF8String]);
	}

	[pool drain];
	return path;
}

std::string osx_GetBundleResourcesPath()
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	std::string path;

	NSBundle *bundle = [NSBundle bundleWithIdentifier: [NSString stringWithUTF8String: BUNDLE_ID_STR]];
	if (bundle != nil)
	{
		NSString *pathStr;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
		// Retrieve NSURL and convert to POSIX path, then get C-string
		//	encoded as UTF-8, and use it to construct std::string
		// NSURL:path "If the receiver does not conform to RFC 1808, returns nil."
		if ([bundle respondsToSelector: @selector(resourceURL)])
			pathStr = [[bundle resourceURL] path];
		else
#endif
			pathStr = [bundle resourcePath];

		if (pathStr != nil)
			path = std::string([pathStr UTF8String]);
	}

	[pool drain];
	return path;
}

std::string osx_GetBundleFrameworksPath()
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	std::string path;

	NSBundle *bundle = [NSBundle bundleWithIdentifier: [NSString stringWithUTF8String: BUNDLE_ID_STR]];
	if (bundle != nil)
	{
		NSString *pathStr;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
		// Retrieve NSURL and convert to POSIX path, then get C-string
		//	encoded as UTF-8, and use it to construct std::string
		// NSURL:path "If the receiver does not conform to RFC 1808, returns nil."
		if ([bundle respondsToSelector: @selector(privateFrameworksURL)])
			pathStr = [[bundle privateFrameworksURL] path];
		else
#endif
			pathStr = [bundle privateFrameworksPath];

		if (pathStr != nil)
			path = std::string([pathStr UTF8String]);
	}

	[pool drain];
	return path;
}
