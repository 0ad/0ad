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

// note: the BFD stuff *could* be used on other platforms, if we saw the
// need for it.

#include "precompiled.h"

#include "lib/sysdep/sysdep.h"
#include "lib/debug.h"

void* debug_GetCaller(void* UNUSED(context), const char* UNUSED(lastFuncToSkip))
{
	return NULL;
}

LibError debug_DumpStack(wchar_t* UNUSED(buf), size_t UNUSED(max_chars), void* UNUSED(context), const char* UNUSED(lastFuncToSkip))
{
	return ERR::NOT_IMPLEMENTED;
}

LibError debug_ResolveSymbol(void* UNUSED(ptr_of_interest), char* UNUSED(sym_name), char* UNUSED(file), int* UNUSED(line))
{
	return ERR::NOT_IMPLEMENTED;
}

void debug_SetThreadName(char const* UNUSED(name))
{
    // Currently unimplemented
}
