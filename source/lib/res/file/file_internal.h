#include "file.h"
#include "file_cache.h"
#include "file_io.h"

#include "file_stats.h"	// must come after file and file_cache

#include "compression.h"
#include "zip.h"
#include "archive.h"

#include "vfs.h"
#include "vfs_path.h"
#include "vfs_mount.h"
#include "vfs_tree.h"

#include "trace.h"
#include "vfs_optimizer.h"

const size_t AIO_SECTOR_SIZE = 512;

// helper routine used by functions that call back to a FileIOCB.
//
// bytes_processed is 0 if return value != { ERR_OK, INFO_CB_CONTINUE }
// note: don't abort if = 0: zip callback may not actually
// output anything if passed very little data.
extern LibError file_io_call_back(const void* block, size_t size,
	FileIOCB cb, uintptr_t ctx, size_t& bytes_processed);
