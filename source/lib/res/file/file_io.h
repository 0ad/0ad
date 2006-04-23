/**
 * =========================================================================
 * File        : file_io.h
 * Project     : 0 A.D.
 * Description : provide fast I/O via POSIX aio and splitting into blocks.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2006 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef FILE_IO_H__
#define FILE_IO_H__

extern void file_io_init();
extern void file_io_shutdown();

extern LibError file_io_get_buf(FileIOBuf* pbuf, size_t size,
	const char* atom_fn, uint file_flags, FileIOCB cb);

#endif	// #ifndef FILE_IO_H__
