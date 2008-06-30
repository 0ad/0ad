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
