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

#ifndef __ZIP_H__
#define __ZIP_H__

#include "res.h"

// open and return a handle to the zip archive indicated by <fn>
extern Handle zip_open(const char* fn);
extern int zip_close(Handle ha);

extern int zip_archive_info(Handle ha, char* fn, Handle* hm);

extern int zip_stat(Handle ha, const char* fn, struct stat* s);


struct ZFILE
{
	size_t ofs;
	size_t csize;
	size_t ucsize;
};

// return information about file <fn> in archive <ha> (i.e. 'open' it)
extern ZFILE* zip_lookup(Handle ha, const char* fn);

extern void* zip_inflate_start(void* out, size_t out_size);
extern int zip_inflate_process(void* ctx, void* p, size_t size);
extern int zip_inflate_end(void* ctx);


#endif	// #ifndef __ZIP_H__