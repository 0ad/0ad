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
#include "dbghelp.h"

// define extension function pointers
extern "C"
{
#define FUNC(ret, name, params) ret (__stdcall *p##name) params;
#include "dbghelp_funcs.h"
#undef FUNC
}

void dbghelp_ImportFunctions()
{
	HMODULE hDbghelp = LoadLibraryW(L"dbghelp.dll");
	debug_assert(hDbghelp);
#define FUNC(ret, name, params) p##name = (ret (__stdcall*) params)GetProcAddress(hDbghelp, #name);
#include "dbghelp_funcs.h"
#undef FUNC
}
