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

#include "../posix/io_posix.h"

namespace ERR
{
	const LibError IO     = -110100;
	const LibError IO_EOF = -110101;
}

// called by file_io after a block IO has completed.
// *bytesProcessed must be set; file_io will return the sum of these values.
// example: when reading compressed data and decompressing in the callback,
// indicate #bytes decompressed.
// return value: INFO::CB_CONTINUE to continue calling; anything else:
//   abort immediately and return that.
// note: in situations where the entire IO is not split into blocks
// (e.g. when reading from cache or not using AIO), this is still called but
// for the entire IO. we do not split into fake blocks because it is
// advantageous (e.g. for decompressors) to have all data at once, if available
// anyway.
typedef LibError (*IoCallback)(uintptr_t cbData, const u8* block, size_t size, size_t* bytesProcessed);


// helper routine used by functions that call back to a IoCallback.
//
// bytesProcessed is 0 if return value != { INFO::OK, INFO::CB_CONTINUE }
// note: don't abort if = 0: zip callback may not actually
// output anything if passed very little data.
extern LibError io_InvokeCallback(const u8* block, size_t size, IoCallback cb, uintptr_t cbData, size_t& bytesProcessed);


#endif	// #ifndef INCLUDED_IO_MANAGER
