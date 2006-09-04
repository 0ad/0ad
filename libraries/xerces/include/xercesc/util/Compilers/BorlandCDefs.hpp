/*
 * Copyright 1999-2002,2004 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id: BorlandCDefs.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(BORLANDCDEFS_HPP)
#define BORLANDCDEFS_HPP

// ---------------------------------------------------------------------------
// The following values represent various compiler version levels stored in
// the precompiler macro __BORLANDC__, along with the associated C++Builder
// IDE and compiler versions.
//
//      __BORLANDC__         IDE             Compiler
//      ------------    --------------  ------------------
//         0x510        BCB1            Borland C++ 5.1
//         0x530        BCB3            Borland C++ 5.3
//         0x540        BCB4            Borland C++ 5.4
//         0x550        BCB5            Borland C++ 5.5
//         0x551        BCB5 (patch 1)  Borland C++ 5.5.1
//
// Thus, a single copy of this file may be used to define a build environment
// for any Borland C++Builder compiler.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//  Include some runtime files that will be needed product wide
// ---------------------------------------------------------------------------
#include <sys/types.h>  // for size_t and ssize_t
#include <limits.h>  // for MAX of size_t and ssize_t

// ---------------------------------------------------------------------------
//  A define in the build for each project is also used to control whether
//  the export keyword is from the project's viewpoint or the client's.
//  These defines provide the platform specific keywords that they need
//  to do this.
// ---------------------------------------------------------------------------
#define PLATFORM_EXPORT     __declspec(dllexport)
#define PLATFORM_IMPORT     __declspec(dllimport)

// ---------------------------------------------------------------------------
//  Indicate that we do not support native bools
//  If the compiler can handle boolean itself, do not define it
// ---------------------------------------------------------------------------
// #define NO_NATIVE_BOOL

// ---------------------------------------------------------------------------
//  Each compiler might support L"" prefixed constants. There are places
//  where it is advantageous to use the L"" where it supported, to avoid
//  unnecessary transcoding.
//  If your compiler does not support it, don't define this.
// ---------------------------------------------------------------------------
#define XML_LSTRSUPPORT

// ---------------------------------------------------------------------------
//  Indicate that we support C++ namespace
//  Do not define it if the compile cannot handle C++ namespace
// ---------------------------------------------------------------------------
#define XERCES_HAS_CPP_NAMESPACE

// ---------------------------------------------------------------------------
//  Define our version of the XML character
// ---------------------------------------------------------------------------
#if (__BORLANDC__ < 0x540)
typedef unsigned short  XMLCh;
#else
typedef wchar_t  XMLCh;
#endif (__BORLANDC__ < 0x540)

// ---------------------------------------------------------------------------
//  Define unsigned 16 and 32 bits integers
// ---------------------------------------------------------------------------
typedef unsigned short  XMLUInt16;
typedef unsigned int    XMLUInt32;

// ---------------------------------------------------------------------------
//  Define signed 32 bits integers
// ---------------------------------------------------------------------------
typedef int             XMLInt32;

// ---------------------------------------------------------------------------
//  XMLSize_t is the unsigned integral type.
// ---------------------------------------------------------------------------
#if defined(_SIZE_T) && defined(SIZE_MAX) && defined(_SSIZE_T) && defined(SSIZE_MAX)
    typedef size_t              XMLSize_t;
    #define XML_SIZE_MAX        SIZE_MAX
    typedef ssize_t             XMLSSize_t;
    #define XML_SSIZE_MAX       SSIZE_MAX
#else
    typedef unsigned long       XMLSize_t;
    #define XML_SIZE_MAX        ULONG_MAX
    typedef long                XMLSSize_t;
    #define XML_SSIZE_MAX       LONG_MAX
#endif


//WLH-2000-08-18 -- Add more compiler quirk cross-defines
#define _itoa  std::itoa


// ---------------------------------------------------------------------------
//  Force on the Xerces debug token if it was on in the build environment
// ---------------------------------------------------------------------------
#ifdef _DEBUG
#define XERCES_DEBUG
#endif


// ---------------------------------------------------------------------------
//  The name of the DLL as built by the Borland projects. At this time, it
//  does not use the versioning stuff.
// ---------------------------------------------------------------------------
const char* const Xerces_DLLName = "XercesLib";

#endif //BORLANDCDEFS_HPP
