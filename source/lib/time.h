// platform indepentend high resolution timer
//
// Copyright (c) 2003 Jan Wassenberg
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

#ifndef __TIME_H__
#define __TIME_H__

#ifdef __cplusplus
extern "C" {
#endif


// high resolution (> 1 µs) timestamp [s], starting at or near 0 s.
extern double get_time();


// calculate fps (call once per frame)
// several smooth filters (tuned for ~100 FPS)
// => less fluctuation, but rapid tracking

extern int fps;

extern void calc_fps();


#ifdef __cplusplus
}
#endif

#endif	// #ifndef __TIME_H__
