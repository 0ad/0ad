/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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

