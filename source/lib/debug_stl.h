extern void stl_simplify_name(char* name);

// no STL iterator is larger than this.
const size_t DEBUG_STL_MAX_ITERATOR_SIZE = 64;

extern int stl_get_container_info(wchar_t* type_name, const u8* p, size_t size,
	size_t el_size, size_t* el_count, DebugIterator* el_iterator, void* it_mem);
