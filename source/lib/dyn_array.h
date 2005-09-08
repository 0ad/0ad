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
