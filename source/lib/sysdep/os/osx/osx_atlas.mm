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

#import <AvailabilityMacros.h> // MAC_OS_X_VERSION_MIN_REQUIRED
#import <AppKit/AppKit.h>

#import "osx_atlas.h"

#include "lib/types.h"
#include "ps/CStr.h"

void startNewAtlasProcess(const std::vector<CStr8>& mods)
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	NSMutableArray *args = [[NSMutableArray alloc] init];
	[args addObject:@"--editor"];

	// Pass mods on the command line.
	for (const CStr8& mod : mods)
	{
		std::string arg = std::string("-mod=") + mod;
		[args addObject:[[NSString alloc] initWithUTF8String:arg.c_str()]];
	}

	// Apple documents this as (deprecated) NSWorkspaceLaunchConfigurationKey, but that's not available in early SDKs.
	NSDictionary<NSString*, id> *params = @{ NSWorkspaceLaunchConfigurationArguments: args };

	[[NSWorkspace sharedWorkspace] launchApplicationAtURL:[[NSRunningApplication currentApplication] executableURL] options:NSWorkspaceLaunchNewInstance configuration:params error:nil];

	[pool drain];
}
