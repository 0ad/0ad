/**
 * =========================================================================
 * File        : io_manager.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_IO_MANAGER
#define INCLUDED_IO_MANAGER

#include "lib/file/posix/io_posix.h"	// File_Posix, IoBuf

// called by file_io after a block IO has completed.
// bytesProcessed must be set; file_io will return the sum of these values.
// example: when reading compressed data and decompressing in the callback,
// indicate #bytes decompressed.
// return value: INFO::CB_CONTINUE to continue calling; anything else:
//   abort immediately and return that.
// note: in situations where the entire IO is not split into blocks
// (e.g. when reading from cache or not using AIO), this is still called but
// for the entire IO. we do not split into fake blocks because it is
// advantageous (e.g. for decompressors) to have all data at once, if available
// anyway.
typedef LibError (*IoCallback)(uintptr_t cbData, const u8* block, size_t size, size_t& bytesProcessed);

extern LibError io_Read(const File_Posix& file, off_t ofs, u8* buf, size_t size, IoCallback cb = 0, uintptr_t cbData = 0);
extern LibError io_Read(const File_Posix& file, off_t ofs, IoBuf buf, size_t size, IoCallback cb = 0, uintptr_t cbData = 0);

extern LibError io_Write(File_Posix& file, off_t ofs, const u8* data, size_t size, IoCallback cb = 0, uintptr_t cbData = 0);
extern LibError io_Write(File_Posix& file, off_t ofs, IoBuf data, size_t size, IoCallback cb = 0, uintptr_t cbData = 0);

#endif	// #ifndef INCLUDED_IO_MANAGER
