/**
 * =========================================================================
 * File        : wdll_ver.h
 * Project     : 0 A.D.
 * Description : return DLL version information
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WDLL_VER
#define INCLUDED_WDLL_VER

#include "lib/os_path.h"

// WARNING: not re-entrant or thread-safe!

// set output buffer into which DLL names and their versions will be written.
extern void wdll_ver_list_init(char* buf, size_t chars);

// read DLL file version and append that and its name to the list.
//
// name should preferably be the complete path to DLL, to make sure
// we don't inadvertently load another one on the library search path.
// we add the .dll extension if necessary.
extern LibError wdll_ver_list_add(const OsPath& name);

#endif	// #ifndef INCLUDED_WDLL_VER
