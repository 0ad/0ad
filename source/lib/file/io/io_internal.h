/**
 * block := power-of-two sized chunk of a file.
 * all transfers are expanded to naturally aligned, whole blocks.
 * (this makes caching parts of files feasible; it is also much faster
 * for some aio implementations, e.g. wposix.)
 * (blocks are also thereby page-aligned, which allows write-protecting
 * file buffers without worrying about their boundaries.)
 **/
static const size_t BLOCK_SIZE = 256*KiB;

// note: *sizes* are aligned to blocks to allow zero-copy block cache.
// that the *buffer* and *offset* must be sector aligned (we assume 4kb for simplicity)
// is a requirement of the OS.
static const size_t SECTOR_SIZE = 4*KiB;

static const unsigned ioDepth = 8;
