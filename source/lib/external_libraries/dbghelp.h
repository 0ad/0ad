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
 * bring in dbghelp library.
 */

#ifndef INCLUDED_DBGHELP
#define INCLUDED_DBGHELP

#include "lib/sysdep/os/win/win.h"

#include <OAIdl.h>	// VARIANT

#if MSC_VERSION
# pragma comment(lib, "oleaut32.lib")	// VariantChangeType
#endif


//-----------------------------------------------------------------------------
// fix omissions in the VC PSDK's dbghelp.h

// the macros defined "for those without specstrings.h" are incorrect -
// parameter definition is missing.
#ifndef __specstrings
# define __specstrings	// prevent dbghelp from changing these

# define __in
# define __out
# define __inout
# define __in_opt
# define __out_opt
# define __inout_opt
# define __in_ecount(s)
# define __out_ecount(s)
# define __inout_ecount(s)
# define __in_bcount(s)
# define __out_bcount(s)
# define __inout_bcount(s)
# define __deref_opt_out
# define __deref_out

#endif

// (VC2005 defines __specstrings, but doesn't define (or use) __out_xcount,
// so this is not inside the above #ifndef section)
//
// missing from dbghelp's list
#ifndef __out_xcount
# define __out_xcount(s)
#endif


//
// not defined by dbghelp; these values are taken from DIA cvconst.h
//

enum BasicType
{
	btNoType    = 0,
	btVoid      = 1,
	btChar      = 2,
	btWChar     = 3,
	btInt       = 6,
	btUInt      = 7,
	btFloat     = 8,
	btBCD       = 9,
	btBool      = 10,
	btLong      = 13,
	btULong     = 14,
	btCurrency  = 25,
	btDate      = 26,
	btVariant   = 27,
	btComplex   = 28,
	btBit       = 29,
	btBSTR      = 30,
	btHresult   = 31
};

enum DataKind
{
	DataIsUnknown,
	DataIsLocal,
	DataIsStaticLocal,
	DataIsParam,
	DataIsObjectPtr,
	DataIsFileStatic,
	DataIsGlobal,
	DataIsMember,
	DataIsStaticMember,
	DataIsConstant
};


//-----------------------------------------------------------------------------

#define _NO_CVCONST_H	// request SymTagEnum be defined
#include <dbghelp.h>	// must come after win.h and the above definitions


//-----------------------------------------------------------------------------

// rationale: Debugging Tools For Windows includes newer header/lib files,
// but they're not redistributable. since we don't want everyone to
// have to install that, it's preferable to link dynamically.

// declare function pointers
#define FUNC(ret, name, params) EXTERN_C ret (__stdcall *p##name) params;
#include "dbghelp_funcs.h"
#undef FUNC

extern void dbghelp_ImportFunctions();

#endif	// #ifndef INCLUDED_DBGHELP
