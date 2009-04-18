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
