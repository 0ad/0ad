// note: the BFD stuff *could* be used on other platforms, if we saw the
// need for it.

#include "precompiled.h"

#include "lib/sysdep/sysdep.h"
#include "lib/debug.h"

void* debug_get_nth_caller(size_t UNUSED(n), void *UNUSED(context))
{
	return NULL;
}

LibError debug_dump_stack(wchar_t* UNUSED(buf), size_t UNUSED(max_chars), size_t UNUSED(skip), void* UNUSED(context))
{
	return ERR::NOT_IMPLEMENTED;
}

LibError debug_resolve_symbol(void* UNUSED(ptr_of_interest), char* UNUSED(sym_name), char* UNUSED(file), int* UNUSED(line))
{
	return ERR::NOT_IMPLEMENTED;
}

void debug_set_thread_name(char const* UNUSED(name))
{
    // Currently unimplemented
}
