#include "precompiled.h"

#include "lib/sysdep/sysdep.h"

#define GNU_SOURCE
#include <dlfcn.h>

LibError sys_get_executable_name(char* n_path, size_t max_chars)
{
	Dl_info dl_info;

	memset(&dl_info, 0, sizeof(dl_info));
	if (!dladdr((void *)sys_get_executable_name, &dl_info) ||
		!dl_info.dli_fname )
	{
		return ERR::NO_SYS;
	}

	strncpy(n_path, dl_info.dli_fname, max_chars);
	return INFO::OK;
}

