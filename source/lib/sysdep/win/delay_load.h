/**
 * =========================================================================
 * File        : delay_load.h
 * Project     : 0 A.D.
 * Description : allow delay-loading DLLs.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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
