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

/**
 * =========================================================================
 * File        : wdll_delay_load.h
 * Project     : 0 A.D.
 * Description : DLL delay loading and notification
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WDLL_DELAY_LOAD
#define INCLUDED_WDLL_DELAY_LOAD

// must be POD because it is used before static ctors run.
struct WdllLoadNotify
{
	const char* dll_name;
	LibError (*func)(void);
	WdllLoadNotify* next;
};

extern void wdll_add_notify(WdllLoadNotify*);

// request that func be called if and when dll_name is ever delay-loaded.
// must be invoked at function scope.
#define WDLL_ADD_NOTIFY(dll_name, func)\
STMT(\
	static WdllLoadNotify UID__ = { dll_name, func };\
	wdll_add_notify(&UID__);\
)

#endif	// #ifndef INCLUDED_WDLL_DELAY_LOAD
