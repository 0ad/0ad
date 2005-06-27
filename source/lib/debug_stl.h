extern void stl_simplify_name(char* name);

// no STL iterator is larger than this; see below.
const size_t DEBUG_STL_MAX_ITERATOR_SIZE = 64;

// if <type_name> indicates the object <p, size> to be an STL container,
// and given the size of its value_type (retrieved via debug information),
// return number of elements and an iterator (any data it needs is stored in
// it_mem, which must hold DEBUG_STL_MAX_ITERATOR_SIZE bytes).
// returns 0 on success, 1 if type_name is unknown, or -1 if the contents
// are invalid (most likely due to being uninitialized).
extern int stl_get_container_info(const wchar_t* type_name, const u8* p, size_t size,
	size_t el_size, size_t* el_count, DebugIterator* el_iterator, void* it_mem);
