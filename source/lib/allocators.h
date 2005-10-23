#include "lib/types.h"

struct DynArray
{
	u8* base;
	size_t max_size_pa;	// reserved
	size_t cur_size;	// committed
	size_t pos;
	int prot;	// applied to newly committed pages
};


extern int da_alloc(DynArray* da, size_t max_size);

extern int da_free(DynArray* da);

extern int da_set_size(DynArray* da, size_t new_size);

extern int da_set_prot(DynArray* da, int prot);

extern int da_wrap_fixed(DynArray* da, u8* p, size_t size);

extern int da_read(DynArray* da, void* data_dst, size_t size);

extern int da_append(DynArray* da, const void* data_src, size_t size);



//
// pool allocator
//

// design goals: O(1) alloc and free; doesn't preallocate the entire pool;
// returns sequential addresses.
//
// (note: this allocator returns fixed-size blocks, the size of which is
// specified at pool_create time. this makes O(1) time possible.) 

// opaque! do not read/write any fields!
struct Pool
{
	DynArray da;
	size_t el_size;

	// all bytes in da up to this mark are in circulation or freelist.
	size_t pos;

	// pointer to freelist (opaque); see freelist_*.
	void* freelist;
};

// ready <p> for use. pool_alloc will return chunks of memory that
// are exactly <el_size> bytes. <max_size> is the upper limit [bytes] on
// pool size (this is how much address space is reserved).
//
// note: el_size must at least be enough for a pointer (due to freelist
// implementation) but not exceed the expand-by amount.
extern int pool_create(Pool* p, size_t max_size, size_t el_size);

// free all memory that ensued from <p>. all elements are made unusable
// (it doesn't matter if they were "allocated" or in freelist or unused);
// future alloc and free calls on this pool will fail.
extern int pool_destroy(Pool* p);

// indicate whether <el> was allocated from the given pool.
// this is useful for callers that use several types of allocators.
extern bool pool_contains(Pool* p, void* el);

// return an entry from the pool, or 0 if it cannot be expanded as necessary.
// exhausts the freelist before returning new entries to improve locality.
extern void* pool_alloc(Pool* p);

// make <el> available for reuse in the given pool.
extern void pool_free(Pool* p, void* el);


//
// bucket allocator
//

struct Bucket
{
	// currently open bucket. must be initialized to 0.
	u8* bucket;

	// offset of free space at end of current bucket (i.e. # bytes in use).
	// must be initialized to 0.
	size_t pos;

	// records # buckets allocated; used to check if the list of them
	// isn't corrupted. must be initialized to 0.
	uint num_buckets;
};


extern void* bucket_alloc(Bucket* b, size_t size);

extern void bucket_free_all(Bucket* b);


//
// matrix allocator
//

// takes care of the dirty work of allocating 2D matrices:
// - aligns data
// - only allocates one memory block, which is more efficient than
//   malloc/new for each row.

// allocate a 2D cols x rows matrix of <el_size> byte cells.
// this must be freed via matrix_free. returns 0 if out of memory.
//
// the returned pointer should be cast to the target type (e.g. int**) and
// can then be accessed by matrix[col][row].
//
extern void** matrix_alloc(uint cols, uint rows, size_t el_size);

// free the given matrix (allocated by matrix_alloc). no-op if matrix == 0.
// callers will likely want to pass variables of a different type
// (e.g. int**); they must be cast to void**.
extern void matrix_free(void** matrix);
