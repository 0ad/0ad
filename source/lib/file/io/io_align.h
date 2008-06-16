#ifndef INCLUDED_IO_ALIGN
#define INCLUDED_IO_ALIGN

#include "lib/bits.h"	// IsAligned, round_up

/**
 * block := power-of-two sized chunk of a file.
 * all transfers are expanded to naturally aligned, whole blocks.
 * (this makes caching parts of files feasible; it is also much faster
 * for some aio implementations, e.g. wposix.)
 * (blocks are also thereby page-aligned, which allows write-protecting
 * file buffers without worrying about their boundaries.)
 **/
static const size_t BLOCK_SIZE = 256*KiB;

// note: *sizes* and *offsets* are aligned to blocks to allow zero-copy block cache.
// that the *buffer* need only be sector-aligned (we assume 4kb for simplicity)
// (this is a requirement of the underlying Windows OS)
static const size_t SECTOR_SIZE = 4*KiB;


template<class T>
bool IsAligned_Data(T* address)
{
	return IsAligned((uintptr_t)address, SECTOR_SIZE);
}

static bool IsAligned_Offset(off_t ofs)
{
	return IsAligned(ofs, BLOCK_SIZE);
}


static off_t AlignedOffset(off_t ofs)
{
	return round_down(ofs, (off_t)BLOCK_SIZE);
}

static off_t AlignedSize(off_t size)
{
	return round_up(size, (off_t)BLOCK_SIZE);
}

static off_t PaddedSize(off_t size, off_t ofs)
{
	return round_up(size + ofs - AlignedOffset(ofs), (off_t)BLOCK_SIZE);
}

#endif	// #ifndef INCLUDED_IO_ALIGN
