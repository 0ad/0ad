// ZIP archiving (on top of ZLib)
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

#ifndef __UNZIP_H__
#define __UNZIP_H__


#include "res.h"

// open and return a handle to the zip archive indicated by <fn>
extern Handle zopen(const char* fn);

// open and return a handle to file <fn> in the zip archive <hz>
extern Handle zopen(Handle hz, const char* fn);

extern void zclose(Handle hz);

extern int zread(Handle hf, void*& p, size_t& size, size_t ofs);


#endif	// #ifndef __UNZIP_H__
