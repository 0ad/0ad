/**
 * =========================================================================
 * File        : dll_ver.h
 * Project     : 0 A.D.
 * Description : return DLL version information.
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

#ifndef DLL_VER_H__
#define DLL_VER_H__

// note: this module is not re-entrant or thread-safe!

// set output buffer into which DLL names and their versions will be written.
extern void dll_list_init(char* buf, size_t chars);

// read DLL file version and append that and its name to the list.
//
// name should preferably be the complete path to DLL, to make sure
// we don't inadvertently load another one on the library search path.
// we add the .dll extension if necessary.
extern LibError dll_list_add(const char* name);

#endif	// #ifndef DLL_VER_H__
