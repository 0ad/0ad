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

// X Window System-specific code

#include "precompiled.h"

#if OS_LINUX || OS_BSD
# define HAVE_X 1
#else
# define HAVE_X 0
#endif

#if HAVE_X

#include "lib/debug.h"
#include "lib/utf8.h"
#include "lib/sysdep/gfx.h"
#include "lib/sysdep/cursor.h"

#include "ps/VideoMode.h"

#define Cursor X__Cursor

#include <Xlib.h>
#include <stdlib.h>
#include <Xatom.h>
#include <Xcursor/Xcursor.h>

#include "SDL.h"
#include "SDL_syswm.h"

#include <algorithm>
#undef Cursor
#undef Status

static Display *g_SDL_Display;
static Window g_SDL_Window;
static wchar_t *selection_data=NULL;
static size_t selection_size=0;

namespace gfx {

Status GetVideoMode(int* xres, int* yres, int* bpp, int* freq)
{
	Display* disp = XOpenDisplay(0);
	if(!disp)
		WARN_RETURN(ERR::FAIL);

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
		*xres = XDisplayWidth(disp, screen);
	if(yres)
		*yres = XDisplayHeight(disp, screen);
	if(bpp)
		*bpp = XDefaultDepth(disp, screen);
	if(freq)
		*freq = 0;
	XCloseDisplay(disp);
	return INFO::OK;
}


Status GetMonitorSize(int& width_mm, int& height_mm)
{
	Display* disp = XOpenDisplay(0);
	if(!disp)
		WARN_RETURN(ERR::FAIL);

	int screen = XDefaultScreen(disp);

	width_mm = XDisplayWidthMM(disp, screen);
	height_mm = XDisplayHeightMM(disp, screen);

	XCloseDisplay(disp);
	return INFO::OK;
}

}	// namespace gfx


static bool get_wminfo(SDL_SysWMinfo& wminfo)
{
	SDL_VERSION(&wminfo.version);

	const int ret = SDL_GetWindowWMInfo(g_VideoMode.GetWindow(), &wminfo);

	if(ret == 1)
		return true;

	if(ret == -1)
	{
		debug_printf("SDL_GetWMInfo failed\n");
		return false;
	}
	if(ret == 0)
	{
		debug_printf("SDL_GetWMInfo is not implemented on this platform\n");
		return false;
	}

	debug_printf("SDL_GetWMInfo returned an unknown value: %d\n", ret);
	return false;
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
	if(!disp)
		return NULL;

	// We use CLIPBOARD as the default, since the CLIPBOARD semantics are much
	// closer to windows semantics.
	Atom selSource=XInternAtom(disp, "CLIPBOARD", False);

	Window selOwner=XGetSelectionOwner(disp, selSource);
	if(selOwner == None)
	{
		// However, since many apps don't use CLIPBOARD, but use PRIMARY instead
		// we use XA_PRIMARY as a fallback clipboard. This is true for xterm,
		// for example.
		selSource=XA_PRIMARY;
		selOwner=XGetSelectionOwner(disp, selSource);
	}
	if(selOwner != None) {
		Atom pty=XInternAtom(disp, "SelectionPropertyTemp", False);
		XConvertSelection(disp, selSource, XA_STRING, pty, selOwner, CurrentTime);
		XFlush(disp);

		Atom type;
		int format=0, result=0;
		unsigned long len=0, bytes_left=0, dummy=0;
		u8 *data=NULL;

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
		if(result != Success)
			debug_printf("clipboard_get: result: %d type:%lu len:%lu format:%d bytes_left:%lu\n",
				result, type, len, format, bytes_left);
		if(result == Success && bytes_left > 0)
		{
			result = XGetWindowProperty (disp, selOwner,
				pty, 0, bytes_left, 0,
				AnyPropertyType, &type, &format,
				&len, &dummy, &data);

			if(result == Success)
			{
				debug_printf("clipboard_get: XGetWindowProperty succeeded, returning data\n");
				debug_printf("clipboard_get: data was: \"%s\", type was %lu, XA_STRING atom is %lu\n", (char *)data, type, XA_STRING);

				if(type == XA_STRING) //Latin-1: Just copy into low byte of wchar_t
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

Status sys_clipboard_free(wchar_t *clip_buf)
{
	free(clip_buf);
	return INFO::OK;
}

/**
 * An SDL Event filter that intercepts other applications' requests for the
 * X selection buffer.
 *
 * @see x11_clipboard_init
 * @see sys_clipboard_set
 */
int clipboard_filter(void* UNUSED(userdata), SDL_Event* event)
{
	/* Pass on all non-window manager specific events immediately */
	/* And do nothing if we don't actually have a clip-out to send out */
	if(event->type != SDL_SYSWMEVENT || !selection_data)
		return 1;

	/* Handle window-manager specific clipboard events */
	/* (Note: libsdl must be compiled with X11 support (SDL_VIDEO_DRIVER_X11 in SDL_config.h) -
	   else you'll get errors like "'struct SDL_SysWMmsg' has no member named 'xevent'") */
	XEvent* xevent = &event->syswm.msg->msg.x11.event;
	switch(xevent->type) {
		/* Copy the selection from our buffer to the requested property, and
		convert to the requested target format */
		case SelectionRequest: {
			XSelectionRequestEvent *req;
			XEvent sevent;

			req = &xevent->xselectionrequest;
			sevent.xselection.type = SelectionNotify;
			sevent.xselection.display = req->display;
			sevent.xselection.selection = req->selection;
			sevent.xselection.target = req->target;
			sevent.xselection.property = None;
			sevent.xselection.requestor = req->requestor;
			sevent.xselection.time = req->time;
			// Simply strip all non-Latin1 characters and replace with '?'
			// We should support XA_UTF8
			if(req->target == XA_STRING)
			{
				size_t size = wcslen(selection_data);
				u8* buf = (u8*)alloca(size);

				for(size_t i = 0; i < size; i++)
				{
					buf[i] = selection_data[i] < 0x100 ? selection_data[i] : '?';
				}

				XChangeProperty(g_SDL_Display, req->requestor, req->property,
					sevent.xselection.target, 8, PropModeReplace,
					buf, size);
				sevent.xselection.property = req->property;
			}
			// TODO Add more target formats
			XSendEvent(g_SDL_Display, req->requestor, False, 0, &sevent);
			XSync(g_SDL_Display, False);
		}
		break;
	}

	return 1;
}

/**
 * Initialization for X clipboard handling, called on-demand by
 * sys_clipboard_set.
 */
Status x11_clipboard_init()
{
	SDL_SysWMinfo info;

	if(get_wminfo(info))
	{
		/* Save the information for later use */
		if(info.subsystem == SDL_SYSWM_X11)
		{
			g_SDL_Display = info.info.x11.display;
			g_SDL_Window = info.info.x11.window;

			/* Enable the special window hook events */
			SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
			SDL_SetEventFilter(clipboard_filter, NULL);

			return INFO::OK;
		}
		else
		{
			return ERR::FAIL;
		}
	}

	return INFO::OK;
}

/**
 * Set the Selection (i.e. "copy")
 *
 * Step-by-step (X11)
 * <ul>
 * <li>Store the selection text in a local buffer
 * <li>Tell the X server that we want to own the selection
 * <li>Listen for Selection events and respond to them as appropriate
 * </ul>
 */
Status sys_clipboard_set(const wchar_t *str)
{
	ONCE(x11_clipboard_init());

	debug_printf("sys_clipboard_set: %s\n", utf8_from_wstring(str).c_str());

	if(selection_data)
	{
		free(selection_data);
		selection_data = NULL;
	}

	selection_size = (wcslen(str)+1)*sizeof(wchar_t);
	selection_data = (wchar_t *)malloc(selection_size);
	wcscpy(selection_data, str);

	// Like for the clipboard_get code above, we rather use CLIPBOARD than
	// PRIMARY - more windows'y behaviour there.
	Atom clipboard_atom = XInternAtom(g_SDL_Display, "CLIPBOARD", False);
	XSetSelectionOwner(g_SDL_Display, clipboard_atom, g_SDL_Window, CurrentTime);
	XSetSelectionOwner(g_SDL_Display, XA_PRIMARY, g_SDL_Window, CurrentTime);

	// SDL2 doesn't have a lockable event thread, so it just uses
	// XSync directly instead of lock_func/unlock_func
	XSync(g_SDL_Display, False);

	return INFO::OK;
}

struct sys_cursor_impl
{
	XcursorImage* image;
	X__Cursor cursor;
};

static XcursorPixel cursor_pixel_to_x11_format(const XcursorPixel& bgra_pixel)
{
	BOOST_STATIC_ASSERT(sizeof(XcursorPixel) == 4 * sizeof(u8));
	XcursorPixel ret;
	u8* dst = reinterpret_cast<u8*>(&ret);
	const u8* b = reinterpret_cast<const u8*>(&bgra_pixel);
	const u8 a = b[3];

	for(size_t i = 0; i < 3; ++i)
		*dst++ = (b[i]) * a / 255;
	*dst = a;
	return ret;
}

Status sys_cursor_create(int w, int h, void* bgra_img, int hx, int hy, sys_cursor* cursor)
{
	debug_printf("sys_cursor_create: using Xcursor to create %d x %d cursor\n", w, h);
	XcursorImage* image = XcursorImageCreate(w, h);
	if(!image)
		WARN_RETURN(ERR::FAIL);

	const XcursorPixel* bgra_img_begin = reinterpret_cast<XcursorPixel*>(bgra_img);
	std::transform(bgra_img_begin, bgra_img_begin + (w*h), image->pixels,
		cursor_pixel_to_x11_format);
	image->xhot = hx;
	image->yhot = hy;

	SDL_SysWMinfo wminfo;
	if(!get_wminfo(wminfo))
		WARN_RETURN(ERR::FAIL);

	sys_cursor_impl* impl = new sys_cursor_impl;
	impl->image = image;
	impl->cursor = XcursorImageLoadCursor(wminfo.info.x11.display, image);
	if(impl->cursor == None)
		WARN_RETURN(ERR::FAIL);

	*cursor = static_cast<sys_cursor>(impl);
	return INFO::OK;
}

// returns a dummy value representing an empty cursor
Status sys_cursor_create_empty(sys_cursor* cursor)
{
	static u8 transparent_bgra[] = { 0x0, 0x0, 0x0, 0x0 };

	return sys_cursor_create(1, 1, static_cast<void*>(transparent_bgra), 0, 0, cursor);
}

// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
Status sys_cursor_set(sys_cursor cursor)
{
	if(!cursor) // restore default cursor
		SDL_ShowCursor(SDL_DISABLE);
	else
	{
		SDL_SysWMinfo wminfo;
		if(!get_wminfo(wminfo))
			WARN_RETURN(ERR::FAIL);

		if(wminfo.subsystem != SDL_SYSWM_X11)
			WARN_RETURN(ERR::FAIL);

		SDL_ShowCursor(SDL_ENABLE);

		Window window;
		if(wminfo.info.x11.window)
			window = wminfo.info.x11.window;
		else
			WARN_RETURN(ERR::FAIL);

		XDefineCursor(wminfo.info.x11.display, window,
			static_cast<sys_cursor_impl*>(cursor)->cursor);
		// SDL2 doesn't have a lockable event thread, so it just uses
		// XSync directly instead of lock_func/unlock_func
		XSync(wminfo.info.x11.display, False);
	}

	return INFO::OK;}

// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
Status sys_cursor_free(sys_cursor cursor)
{
	// bail now to prevent potential confusion below; there's nothing to do.
	if(!cursor)
		return INFO::OK;

	sys_cursor_set(0); // restore default cursor
	sys_cursor_impl* impl = static_cast<sys_cursor_impl*>(cursor);

	XcursorImageDestroy(impl->image);

	SDL_SysWMinfo wminfo;
	if(!get_wminfo(wminfo))
		return ERR::FAIL;
	XFreeCursor(wminfo.info.x11.display, impl->cursor);

	delete impl;

	return INFO::OK;
}

Status sys_cursor_reset()
{
	return INFO::OK;
}


#endif	// #if HAVE_X
