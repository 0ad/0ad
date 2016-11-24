/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * wrapper for Windows Management Instrumentation
 */

#ifndef INCLUDED_WMI
#define INCLUDED_WMI

#include <map>

// note: we expose the VARIANT value as returned by WMI. this allows other
// modules to use the values they want directly, rather than forcing
// everything to be converted to/parsed from strings. it does drag in
// OLE headers, but this module is entirely Windows-specific anyway.
#define _WIN32_DCOM
#include "lib/sysdep/os/win/win.h"
#include <comdef.h>	// VARIANT
// contains name and value of all instance properties
typedef std::map<std::wstring, VARIANT> WmiInstance;

typedef std::vector<WmiInstance> WmiInstances;

/**
 * get all instances of the requested class.
 **/
extern Status wmi_GetClassInstances(const wchar_t* className, WmiInstances& instances);

extern void wmi_Shutdown();

#endif	// #ifndef INCLUDED_WMI
