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

#if ICC_VERSION
# pragma warning(push)
# pragma warning(disable:94)	// the size of an array must be greater than zero
# pragma warning(disable:271)	// trailing comma is nonstandard
# pragma warning(disable:418)	// declaration requires a typedef name
# pragma warning(disable:791)	// calling convention specified more than once
#endif

#pragma pack(push, 8)	// seems to be required

#define _NO_CVCONST_H	// request SymTagEnum be defined
#include <dbghelp.h>	// must come after win.h and the above definitions

#pragma pack(pop)

#if ICC_VERSION
# pragma warning(pop)
#endif


//-----------------------------------------------------------------------------

// rationale: Debugging Tools For Windows includes newer header/lib files,
// but they're not redistributable. since we don't want everyone to
// have to install that, it's preferable to link dynamically.

// declare function pointers
#define FUNC(ret, name, params) EXTERN_C ret (__stdcall *p##name) params;
#include "lib/external_libraries/dbghelp_funcs.h"
#undef FUNC

extern void dbghelp_ImportFunctions();

#endif	// #ifndef INCLUDED_DBGHELP
