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

#ifdef HAVE_X

#include <Xlib.h>

// useful for choosing a video mode. not called by detect().
// if we fail, don't change the outputs (assumed initialized to defaults)
void get_cur_resolution(int& xres, int& yres)
{
	Display* disp = XOpenDisplay(0);
	if(!disp)
		return;

	int screen = XDefaultScreen(disp);
	xres = XDisplayWidth (disp, screen);
	yres = XDisplayHeight(disp, screen);
	XCloseDisplay(disp);
}

#endif	// #ifdef HAVE_X