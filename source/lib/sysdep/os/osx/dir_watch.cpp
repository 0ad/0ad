#include "precompiled.h"
#include "lib/sysdep/dir_watch.h"

// stub implementations

LibError dir_add_watch(
		const char * const UNUSED(n_full_path),
		intptr_t* const UNUSED(watch))
{
	return INFO::OK;
}

LibError dir_cancel_watch(const intptr_t UNUSED(watch))
{
	return INFO::OK;
}

LibError dir_get_changed_file(char *)
{
	return ERR::AGAIN; // Say no files are available.
}
