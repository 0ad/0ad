
struct BlockId
{
	const char* atom_fn;
	u32 block_num;
};

extern bool block_eq(BlockId b1, BlockId b2);

// create an id for use with the cache that uniquely identifies
// the block from the file <atom_fn> starting at <ofs>.
extern BlockId block_cache_make_id(const char* atom_fn, const off_t ofs);

extern void* block_cache_alloc(BlockId id);

extern void block_cache_mark_completed(BlockId id);

extern void* block_cache_find(BlockId id);
extern void block_cache_release(BlockId id);




extern LibError file_buf_get(FileIOBuf* pbuf, size_t size,
	const char* atom_fn, bool is_write, FileIOCB cb);

extern LibError file_buf_set_real_fn(FileIOBuf buf, const char* atom_fn);

extern FileIOBuf file_cache_find(const char* atom_fn, size_t* size);
extern FileIOBuf file_cache_retrieve(const char* atom_fn, size_t* size);
extern LibError file_cache_add(FileIOBuf buf, size_t size, const char* atom_fn);


extern void file_cache_init();
extern void file_cache_shutdown();
