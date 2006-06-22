/**
 * =========================================================================
 * File        : delay_load.h
 * Project     : 0 A.D.
 * Description : allow delay-loading DLLs.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

struct DllLoadNotify;

extern void wdll_add_notify(DllLoadNotify*);

// note: this mechanism relies on the compiler calling non-local static
// object ctors, which doesn't happen if compiling this code into
// a static library. recommended workaround is to call wdll_add_notify via
// win.cpp module init mechanism.
struct DllLoadNotify
{
	const char* dll_name;
	LibError (*func)(void);
	DllLoadNotify* next;

	DllLoadNotify(const char* _dll_name, LibError (*_func)(void))
	{
		dll_name = _dll_name;
		func = _func;
		wdll_add_notify(this);
	}
};

#define WDLL_LOAD_NOTIFY(dll_name, func)\
	static DllLoadNotify func##_NOTIFY(dll_name, func)
