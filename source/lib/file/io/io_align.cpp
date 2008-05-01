#include "precompiled.h"
#include "io_align.h"


bool IsAligned_Offset(off_t ofs)
{
	return IsAligned(ofs, BLOCK_SIZE);
}

off_t AlignedOffset(off_t ofs)
{
	return round_down(ofs, (off_t)BLOCK_SIZE);
}

off_t AlignedSize(off_t size)
{
	return round_up(size, (off_t)BLOCK_SIZE);
}

off_t PaddedSize(off_t size, off_t ofs)
{
	return round_up(size + ofs - AlignedOffset(ofs), (off_t)BLOCK_SIZE);
}
