#ifndef INCLUDED_RES
#define INCLUDED_RES

// common headers needed by lib/res code

#include "h_mgr.h"
#include "lib/res/file/vfs.h"
#include "lib/path_util.h"
#include "mem.h"


namespace ERR
{
	const LibError RES_UNKNOWN_FORMAT    = -120000;
	const LibError RES_INCOMPLETE_HEADER = -120001;
}

#endif	// #ifndef INCLUDED_RES
