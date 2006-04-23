/**
 * =========================================================================
 * File        : file_internal.h
 * Project     : 0 A.D.
 * Description : master (private) header for all file code.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2006 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "lib/path_util.h"
#include "path.h"

#include "file.h"
#include "file_cache.h"
#include "file_io.h"

#include "file_stats.h"	// must come after file and file_cache

#include "compression.h"
#include "zip.h"
#include "archive.h"

#include "vfs.h"
#include "vfs_mount.h"
#include "vfs_tree.h"
#include "vfs_redirector.h"

#include "trace.h"
#include "vfs_optimizer.h"

const size_t AIO_SECTOR_SIZE = 512;

// block := power-of-two sized chunk of a file.
// all transfers are expanded to naturally aligned, whole blocks
// (this makes caching parts of files feasible; it is also much faster
// for some aio implementations, e.g. wposix).
//
// this is not exposed to users because it's an implementation detail and
// they shouldn't care.
//
// measurements show this value to yield best read throughput.
const size_t FILE_BLOCK_SIZE = 32*KiB;

// helper routine used by functions that call back to a FileIOCB.
//
// bytes_processed is 0 if return value != { ERR_OK, INFO_CB_CONTINUE }
// note: don't abort if = 0: zip callback may not actually
// output anything if passed very little data.
extern LibError file_io_call_back(const void* block, size_t size,
	FileIOCB cb, uintptr_t ctx, size_t& bytes_processed);

// used by file.cpp and file_io.cpp
struct PosixFile
{
	int fd;

	// for reference counted memory-mapping
	void* mapping;
	uint map_refs;
};
