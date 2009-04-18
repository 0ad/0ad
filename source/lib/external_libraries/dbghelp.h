/**
 * =========================================================================
 * File        : dbghelp.h
 * Project     : 0 A.D.
 * Description : bring in dbghelp library
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_DBGHELP
#define INCLUDED_DBGHELP

#include "win.h"

#define _NO_CVCONST_H	// request SymTagEnum be defined
#include <dbghelp.h>	// must come after win.h
#include <OAIdl.h>	// VARIANT

#if MSC_VERSION
# pragma comment(lib, "dbghelp.lib")
# pragma comment(lib, "oleaut32.lib")	// VariantChangeType
#endif

#endif	// #ifndef INCLUDED_DBGHELP
