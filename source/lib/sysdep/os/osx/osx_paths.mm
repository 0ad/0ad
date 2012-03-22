/* Copyright (c) 2012 Wildfire Games
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

#import <Foundation/Foundation.h>
#import <string>

#import "osx_paths.h"

// Helper function
static std::string getUserDirectoryPath(NSSearchPathDirectory directory)
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	std::string result;

	// Returns array of NSURL objects which are preferred for file paths
	NSArray* paths = [[NSFileManager defaultManager] URLsForDirectory:directory inDomains:NSUserDomainMask];
	if ([paths count] > 0)
	{
		// Retrieve first NSURL and convert to POSIX path, then get C-string
		//	encoded as UTF-8, and use it to construct std::string
		// NSURL:path "If the receiver does not conform to RFC 1808, returns nil."
		NSString* pathStr = [[paths objectAtIndex:0] path];
		if (pathStr != nil)
			result = std::string([pathStr UTF8String]);
	}

	[pool drain];
	return result;
}

std::string osx_GetAppSupportPath()
{
	return getUserDirectoryPath(NSApplicationSupportDirectory);
}

std::string osx_GetCachesPath()
{
	return getUserDirectoryPath(NSCachesDirectory);
}
