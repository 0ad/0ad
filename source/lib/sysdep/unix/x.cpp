// X Window System-specific code
// Copyright (c) 2004 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#ifdef HAVE_X

#include <Xlib.h>
#include <stdlib.h>
#include <Xatom.h>

#include "lib.h"
#include "debug.h"
#include "sysdep/gfx.h"

#include <algorithm>

// useful for choosing a video mode. not called by detect().
// if we fail, outputs are unchanged (assumed initialized to defaults)
LibError gfx_get_video_mode(int* xres, int* yres, int* bpp, int* freq)
{
	Display* disp = XOpenDisplay(0);
	if(!disp)
		return ERR_FAIL;

	int screen = XDefaultScreen(disp);
	
	/* 2004-07-13
	NOTE: The XDisplayWidth/Height functions don't actually return the current
	display mode - they return the size of the root window. This means that
	users with "Virtual Desktops" bigger than what their monitors/graphics
	card can handle will have to set their 0AD screen resolution manually.
	
	There's supposed to be an X extension that can give you the actual display
	mode, probably including refresh rate info etc, but it's not worth
	researching and implementing that at this stage.
	*/
	
	if(xres)
		*xres = XDisplayWidth (disp, screen);
	if(yres)
		*yres = XDisplayHeight(disp, screen);
	if(bpp)
		*bpp = XDefaultDepth(disp, screen);
	if(freq)
		*freq = 0;
	XCloseDisplay(disp);
	return ERR_OK;
}


// useful for determining aspect ratio. not called by detect().
// if we fail, outputs are unchanged (assumed initialized to defaults)
LibError gfx_get_monitor_size(int& width_mm, int& height_mm)
{
	Display* disp = XOpenDisplay(0);
	if(!disp)
		return ERR_FAIL;

	int screen = XDefaultScreen(disp);
	
	width_mm=XDisplayWidthMM(disp, screen);
	height_mm=XDisplayHeightMM(disp, screen);
	
	XCloseDisplay(disp);
	return ERR_OK;
}

/*
Oh, boy, this is heavy stuff...

<User-End Stuff - Definitions and Conventions>
http://www.freedesktop.org/standards/clipboards-spec/clipboards.txt
<Technical, API stuff>
http://www.mail-archive.com/xfree86@xfree86.org/msg15594.html
http://michael.toren.net/mirrors/doc/X-copy+paste.txt
http://devdocs.wesnoth.org/clipboard_8cpp-source.html
http://tronche.com/gui/x/xlib/window-information/XGetWindowProperty.html
http://www.jwz.org/doc/x-cut-and-paste.html

The basic run-down on X Selection Handling:
* One window owns the "current selection" at any one time
* Accessing the Selection (i.e. "paste"), Step-by-step
	* Ask the X server for the current selection owner
	* Ask the selection owner window to convert the selection into a format
	we can understand (XA_STRING - Latin-1 string - for now)
	* The converted result is stored as a property of the *selection owner*
	window. It is possible to specify the current application window as the
	target - but that'd require some X message handling... easier to skip that..
	* The final step is to acquire the property value of the selection owner
	window

Notes:
An "Atom" is a server-side object that represents a string by an index into some
kind of table or something. Pretty much like a handle that refers to one unique
string. Atoms are used here to refer to property names and value formats.

Expansions:
* Implement UTF-8 format support (should be interresting for international users)

*/
wchar_t *sys_clipboard_get()
{
	Display *disp=XOpenDisplay(NULL);
	if (!disp)
		return NULL;

	// We use CLIPBOARD as the default, since the CLIPBOARD semantics are much
	// closer to windows semantics.
	Atom selSource=XInternAtom(disp, "CLIPBOARD", False);

	Window selOwner=XGetSelectionOwner(disp, selSource);
	if (selOwner == None)
	{
		// However, since many apps don't use CLIPBOARD, but use PRIMARY instead
		// we use XA_PRIMARY as a fallback clipboard. This is true for xterm,
		// for example.
		selSource=XA_PRIMARY;
		selOwner=XGetSelectionOwner(disp, selSource);
	}
	if (selOwner != None) {
		Atom pty=XInternAtom(disp, "SelectionPropertyTemp", False);
		XConvertSelection(disp, selSource, XA_STRING, pty, selOwner, CurrentTime);
		XFlush(disp);
		
		Atom type;
		int format=0, result=0;
		unsigned long len=0, bytes_left=0, dummy=0;
		unsigned char *data=NULL;
		
		// Get the length of the property and some attributes
		// bytes_left will contain the length of the selection
		result = XGetWindowProperty (disp, selOwner, pty,
			0, 0,			// offset - len
			0,				// Delete 0==FALSE
			AnyPropertyType,//flag
			&type,			// return type
			&format,		// return format
			&len, &bytes_left,
			&data);
		if (result != Success)
			debug_printf("clipboard_get: result: %d type:%d len:%d format:%d bytes_left:%d\n", 
				result, (int)type, len, format, bytes_left);
		if (result == Success && bytes_left > 0)
		{
			result = XGetWindowProperty (disp, selOwner, 
				pty, 0, bytes_left, 0,
				AnyPropertyType, &type, &format,
				&len, &dummy, &data);
			
			if (result == Success)
			{
				debug_printf("clipboard_get: XGetWindowProperty succeeded, returning data\n");
				debug_printf("clipboard_get: data was: \"%s\", type was %d, XA_STRING atom is %d\n", data, type, XA_STRING);
				
				if (type == XA_STRING) //Latin-1: Just copy into low byte of wchar_t
				{
					wchar_t *ret=(wchar_t *)malloc((bytes_left+1)*sizeof(wchar_t));
					std::copy(data, data+bytes_left, ret);
					ret[bytes_left]=0;
					return ret;
				}
				// TODO: Handle UTF8 strings
			}
			else
			{
				debug_printf("clipboard_get: XGetWindowProperty failed!\n");
				return NULL;
			}
		}
	}
	
	return NULL;
}

LibError sys_clipboard_free(wchar_t *clip_buf)
{
	free(clip_buf);
	return ERR_OK;
}

/*
Setting the Selection (i.e. "copy")
* Would require overriding the SDL X event handler to receive other apps'
  requests for the selection buffer
* Step-by-step:
	* Store the selection text in a local buffer
	* Tell the X server that we want to own the selection
	* Listen for Selection events and respond to them as appropriate
*/
LibError sys_clipboard_set(const wchar_t *clip_str)
{
	// Not Implemented, see comment before clipboard_get, above
	return ERR_FAIL;
}

#endif	// #ifdef HAVE_X
