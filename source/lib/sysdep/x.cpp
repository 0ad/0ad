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
#include "detect.h"

// useful for choosing a video mode. not called by detect().
// if we fail, outputs are unchanged (assumed initialized to defaults)
int get_cur_vmode(int* xres, int* yres, int* bpp, int* freq)
{
	Display* disp = XOpenDisplay(0);
	if(!disp)
		return -1;

	int screen = XDefaultScreen(disp);
	if(xres)
		*xres = XDisplayWidth (disp, screen);
	if(yres)
		*yres = XDisplayHeight(disp, screen);
	if(bpp)
		*bpp = 0;
	if(freq)
		*freq = 0;
	XCloseDisplay(disp);
	return 0;
}


// useful for determining aspect ratio. not called by detect().
// if we fail, outputs are unchanged (assumed initialized to defaults)
int get_monitor_size(int& width_mm, int& height_mm)
{
	return -1;
}


#endif	// #ifdef HAVE_X
