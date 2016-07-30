/* Copyright (c) 2013 Wildfire Games
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

#import <AppKit/AppKit.h>
#import <AvailabilityMacros.h> // MAC_OS_X_VERSION_MIN_REQUIRED
#import <Foundation/Foundation.h>
#import <string>

#import "osx_pasteboard.h"

bool osx_GetStringFromPasteboard(std::string& out)
{
	NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
	NSString* string = nil;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
	// As of 10.6, pasteboards can hold multiple items
	if ([pasteboard respondsToSelector: @selector(readObjectsForClasses:)])
	{
		NSArray* classes = [NSArray arrayWithObjects:[NSString class], nil];
		NSDictionary* options = [NSDictionary dictionary];
		NSArray* copiedItems = [pasteboard readObjectsForClasses:classes options:options];
		// We only need to support a single item, so grab the first string
		if (copiedItems != nil && [copiedItems count] > 0)
			string = [copiedItems objectAtIndex:0];
		else
			return false; // No strings found on pasteboard
	}
	else
	{
#endif 	// fallback to 10.5 API
		// Verify that there is a string available for us
		NSArray* types = [NSArray arrayWithObjects:NSStringPboardType, nil];
		if ([pasteboard availableTypeFromArray:types] != nil)
			string = [pasteboard stringForType:NSStringPboardType];
		else
			return false; // No strings found on pasteboard
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
	}
#endif

	if (string != nil)
		out = std::string([string UTF8String]);
	else
		return false; // fail

	return true; // success
}

bool osx_SendStringToPasteboard(const std::string& string)
{
	// We're only working with strings, so we don't need to lazily write
	// anything (otherwise we'd need to set up an owner and data provider)
	NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
	NSString* type;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
	if ([pasteboard respondsToSelector: @selector(clearContents)])
	{
		type = NSPasteboardTypeString;
		[pasteboard clearContents];
	}
	else
	{
#endif 	// fallback to 10.5 API
		type = NSStringPboardType;
		NSArray* types = [NSArray arrayWithObjects: type, nil];
		// Roughly equivalent to clearContents followed by addTypes:owner
		[pasteboard declareTypes:types owner:nil];
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060

	}
#endif

	// May raise a NSPasteboardCommunicationException
	BOOL ok = [pasteboard setString:[NSString stringWithUTF8String:string.c_str()] forType:type];
	return ok == YES;
}
