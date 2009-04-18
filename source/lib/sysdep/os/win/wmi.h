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

/*
 * wrapper for Windows Management Instrumentation
 */

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
