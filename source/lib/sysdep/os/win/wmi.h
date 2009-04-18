/**
 * =========================================================================
 * File        : wmi.h
 * Project     : 0 A.D.
 * Description : wrapper for Windows Management Instrumentation
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WMI
#define INCLUDED_WMI

// note: we expose the VARIANT value as returned by WMI. this allows other
// modules to use the values they want directly, rather than forcing
// everything to be converted to/parsed from strings. it does drag in
// OLE headers, but this module is entirely Windows-specific anyway.
#define _WIN32_DCOM
#include "win.h"
#include <comdef.h>	// VARIANT
typedef std::map<std::wstring, VARIANT> WmiMap;

/**
 * return a map of name/value pairs of the WMI class members.
 * @return LibError
 **/
extern LibError wmi_GetClass(const char* className, WmiMap& wmiMap);

extern void wmi_Shutdown();

#endif	// #ifndef INCLUDED_WMI
