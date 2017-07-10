/* Copyright (C) 2012 Wildfire Games.
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

#import "precompiled.h"
#import "lib/sysdep/cursor.h"

#import <AppKit/NSCursor.h>
#import <AppKit/NSImage.h>
#import <ApplicationServices/ApplicationServices.h>

//TODO: make sure these are threadsafe
Status sys_cursor_create(int w, int h, void* bgra_img, int hx, int hy, sys_cursor* cursor)
{	
	NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc]
								initWithBitmapDataPlanes:0 pixelsWide:w pixelsHigh:h 
								bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO 
								colorSpaceName:NSCalibratedRGBColorSpace bytesPerRow:w*4 bitsPerPixel:0];
	if (!bitmap)
	{
		debug_printf("sys_cursor_create: Error creating NSBitmapImageRep!\n");
		return ERR::FAIL;
	}

	u8* planes[5];
	[bitmap getBitmapDataPlanes:planes];
	const u8* bgra = static_cast<const u8*>(bgra_img);
	u8* dst = planes[0];
	for (int i = 0; i < w*h*4; i += 4) 
	{
		dst[i] = bgra[i+2];
		dst[i+1] = bgra[i+1];
		dst[i+2] = bgra[i];
		dst[i+3] = bgra[i+3];
	}

	NSImage* image = [[NSImage alloc] init];
	if (!image)
	{
		[bitmap release];
		debug_printf("sys_cursor_create: Error creating NSImage!\n");
		return ERR::FAIL;
	}

	[image addRepresentation:bitmap];
	[bitmap release];
	NSCursor* impl = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(hx, hy)];
	[image release];

	if (!impl)
	{
		debug_printf("sys_cursor_create: Error creating NSCursor!\n");
		return ERR::FAIL;
	}

	*cursor = static_cast<sys_cursor>(impl);

	return INFO::OK;
}

Status sys_cursor_free(sys_cursor cursor)
{
	NSCursor* impl = static_cast<NSCursor*>(cursor);
	[impl release];
	return INFO::OK;
}

Status sys_cursor_create_empty(sys_cursor* cursor)
{
	static u8 empty_bgra[] = {0, 0, 0, 0};
	sys_cursor_create(1, 1, reinterpret_cast<void*>(empty_bgra), 0, 0, cursor);
	return INFO::OK;
}

Status sys_cursor_set(sys_cursor cursor)
{
	NSCursor* impl = static_cast<NSCursor*>(cursor);
	[impl set];
	return INFO::OK;
}

Status sys_cursor_reset()
{
	return INFO::OK;
}

