// platform-independent high resolution timer and FPS measuring code
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

#ifndef TIMER_H
#define TIMER_H

#include <string>

#include "sysdep/debug.h"	// debug_printf

#ifdef __cplusplus
extern "C" {
#endif

// high resolution (> 1 µs) timestamp [s], starting at or near 0 s.
extern double get_time(void);

extern double timer_res(void);

// calculate fps (call once per frame)
// several smooth filters (tuned for ~100 FPS)
// => less fluctuation, but rapid tracking

extern int fps;

extern void calc_fps(void);

#ifdef __cplusplus
}
#endif



class ScopedTimer
{
	double t0;
	const std::string name;

public:
	ScopedTimer(const char* _name)
		: name(_name)
	{
		t0 = get_time();
	}
	~ScopedTimer()
	{
		double t1 = get_time();
		double dt = t1-t0;

		// assume microseconds
		double scale = 1e6;
		char unit = 'µ';
		if(dt > 1.0)
			scale = 1, unit = ' ';
		// milli
		else if(dt > 1e-3)
			scale = 1e3, unit = 'm';

		debug_printf("TIMER %s: %g %cs\n", name.c_str(), dt*scale, unit);
	}

	// no copy ctor, since some members are const
private:
	ScopedTimer& operator=(const ScopedTimer&);
};

#define TIMER(name) ScopedTimer st##name##instance(#name);

#endif	// #ifndef TIMER_H
