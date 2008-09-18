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

/**
 * read DLL version information and append it to a string.
 *
 * @param pathname of DLL (preferably the complete path, so that we don't
 * inadvertently load another one on the library search path.)
 * if no extension is given, .dll will be appended.
 *
 * the text output includes the module name.
 * on failure, the version is given as "unknown".
 **/
extern void wdll_ver_Append(const OsPath& pathname, std::string& list);

#endif	// #ifndef INCLUDED_WDLL_VER
