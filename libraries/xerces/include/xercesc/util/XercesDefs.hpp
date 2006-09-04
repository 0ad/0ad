/*
 * Copyright 1999-2001,2004 The Apache Software Foundation.
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
 * $Log$
 * Revision 1.20  2005/04/27 18:21:51  cargilld
 * Fix for problem on Solaris where open may return 0 as a valid FileHandle.  Check for -1 instead.
 *
 * Revision 1.19  2004/09/08 13:56:25  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.18  2004/02/24 22:57:28  peiyongz
 * XercesDeprecatedDOMLib
 *
 * Revision 1.17  2004/02/17 15:56:50  neilg
 * fix for bug 25035; much thanks to Abe Backus
 *
 * Revision 1.16  2004/02/04 13:26:44  amassari
 * Added support for the Interix platform (Windows Services for Unix 3.5)
 *
 * Revision 1.15  2003/05/29 11:18:37  gareth
 * Added macros in so we can determine whether to do things like iostream as opposed to iostream.h and whether to use std:: or not.
 *
 * Revision 1.14  2003/05/12 09:44:19  gareth
 * Port to NetBSD. Patch by Hiramatsu Yoshifumi.
 *
 * Revision 1.13  2003/03/13 22:11:46  tng
 * [Bug 17858] Support for QNX/Neutrino.  Patch from Chris McKillop.
 *
 * Revision 1.12  2003/02/23 05:44:12  jberry
 * Ripple through changes of BeOSDefs.h name change
 *
 * Revision 1.11  2002/12/31 19:31:07  tng
 * [Bug 15590] BeOSDefs.hpp has wrong case in CVS.
 *
 * Revision 1.10  2002/12/02 20:40:49  tng
 * [Bug 12490] Patches required to build Xerces-C++ on BeOS R5.  Patch from Andrew Bachmann.
 *
 * Revision 1.9  2002/11/05 21:44:21  tng
 * Do not code using namespace in a global header.
 *
 * Revision 1.8  2002/11/04 14:40:12  tng
 * C++ Namespace Support.
 *
 * Revision 1.7  2002/07/12 16:48:49  jberry
 * Remove reliance on XML_MACOSX. XML_MACOS is used solely. Where qualification
 * by compiler is required, look for the compiler explicitly such as with
 * XML_METROWERKS or __APPLE__ (for the Apple GCC compiler).
 *
 * Add a few tweaks for compatibility with GCC3.1.
 *
 * This change may address Bug 10649.
 *
 * Revision 1.6  2002/07/10 12:56:45  tng
 * [Bug 9154] Requesting Xerces Version Macro.
 *
 * Revision 1.5  2002/05/21 19:35:08  tng
 * Update from 1.7 to 2.0
 *
 * Revision 1.4  2002/02/27 22:38:34  peiyongz
 * Bug# 6445 Caldera (SCO) OpenServer Port : patch from Martin Kalen
 *
 * Revision 1.3  2002/02/20 21:41:54  tng
 * project files changes for Xerces-C++ 1.7.
 *
 * Revision 1.2  2002/02/17 21:12:06  jberry
 * Adjust "sane includes" include path for Mac OS.
 *
 * I've also changed this path for XML_AS400, XML_TRU64, XML_PTX_CC, and XML_DECCXX
 * 'cause it looks like the right thing to do...hope that's not a mistake.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:13  peiyongz
 * sane_include
 *
 * Revision 1.18  2001/11/29 18:25:18  tng
 * FreeBSD support by Michael Huedepohl.
 *
 * Revision 1.17  2001/11/23 17:19:33  tng
 * Change from 1.5.2 to 1.6.0
 *
 * Revision 1.16  2001/10/15 16:27:35  tng
 * Changes for Xerces-C 1.5.2
 *
 * Revision 1.15  2001/07/13 20:16:38  tng
 * Update for release 1.5.1.
 *
 * Revision 1.14  2001/06/05 13:52:25  tng
 * Change Version number from Xerces 1.4 to 1.5.  By Pei Yong Zhang.
 *
 * Revision 1.13  2001/05/11 13:26:32  tng
 * Copyright update.
 *
 * Revision 1.12  2001/02/08 14:15:33  tng
 * enable COMPAQ Tru64 UNIX machines to build xerces-c with gcc (tested using COMPAQ gcc version2.95.2 19991024 (release) and Tru64 V5.0 1094).  Added by Martin Kalen.
 *
 * Revision 1.11  2001/01/25 19:17:06  tng
 * const should be used instead of static const.  Fixed by Khaled Noaman.
 *
 * Revision 1.10  2001/01/12 22:09:07  tng
 * Various update for Xerces 1.4
 *
 * Revision 1.9  2000/11/07 18:14:39  andyh
 * Fix incorrect version number in gXercesMinVersion.
 * From Pieter Van-Dyck
 *
 * Revision 1.8  2000/11/02 07:23:27  roddey
 * Just a test of checkin access
 *
 * Revision 1.7  2000/08/18 21:29:14  andyh
 * Change version to 1.3 in preparation for upcoming Xerces 1.3
 * and XML4C 3.3 stable releases
 *
 * Revision 1.6  2000/08/07 20:31:34  jpolast
 * include SAX2_EXPORT module
 *
 * Revision 1.5  2000/08/01 18:26:02  aruna1
 * Tru64 support added
 *
 * Revision 1.4  2000/07/29 05:36:37  jberry
 * Fix misspelling in Mac OS port
 *
 * Revision 1.3  2000/07/19 18:20:12  andyh
 * Macintosh port: fix problems with yesterday's code checkin.  A couple
 * of the changes were mangled or missed.
 *
 * Revision 1.2  2000/04/04 20:11:29  abagchi
 * Added PTX support
 *
 * Revision 1.1  2000/03/02 19:54:50  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.13  2000/03/02 01:51:00  aruna1
 * Sun CC 5.0 related changes
 *
 * Revision 1.12  2000/02/24 20:05:26  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.11  2000/02/22 01:00:10  aruna1
 * GNUGDefs references removed. Now only GCCDefs is used instead
 *
 * Revision 1.10  2000/02/06 07:48:05  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.9  2000/02/01 23:43:32  abagchi
 * AS/400 related change
 *
 * Revision 1.8  2000/01/21 22:12:29  abagchi
 * OS390 Change: changed OE390 to OS390
 *
 * Revision 1.7  2000/01/14 01:18:35  roddey
 * Added a macro, XMLStrL(), which is defined one way or another according
 * to whether the per-compiler file defines XML_LSTRSUPPORT or not. This
 * allows conditional support of L"" type prefixes.
 *
 * Revision 1.6  2000/01/14 00:52:06  roddey
 * Updated the version information for the next release, i.e. 1.1.0
 *
 * Revision 1.5  1999/12/17 01:28:53  rahulj
 * Merged in changes submitted for UnixWare 7 port. Platform
 * specific files are still missing.
 *
 * Revision 1.4  1999/12/16 23:47:10  rahulj
 * Updated for version 1.0.1
 *
 * Revision 1.3  1999/12/01 17:16:16  rahulj
 * Added support for IRIX 6.5.5 using SGI MIPSpro C++ 7.3 and 7.21 generating 32 bit objects. Changes submitted by Marc Stuessel
 *
 * Revision 1.2  1999/11/10 02:02:51  abagchi
 * Changed version numbers
 *
 * Revision 1.1.1.1  1999/11/09 01:05:35  twl
 * Initial checkin
 *
 * Revision 1.3  1999/11/08 20:45:19  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(XERCESDEFS_HPP)
#define XERCESDEFS_HPP

// ---------------------------------------------------------------------------
//  Include the Xerces version information; this is kept in a separate file to
//  make modification simple and obvious. Updates to the version header file
// ---------------------------------------------------------------------------
#include    <xercesc/util/XercesVersion.hpp>


// ---------------------------------------------------------------------------
//  Include the header that does automatic sensing of the current platform
//  and compiler.
// ---------------------------------------------------------------------------
#include    <xercesc/util/AutoSense.hpp>

#define XERCES_Invalid_File_Handle 0

// ---------------------------------------------------------------------------
//  According to the platform we include a platform specific file. This guy
//  will set up any platform specific stuff, such as character mode.
// ---------------------------------------------------------------------------
#if defined(XML_WIN32)
#include    <xercesc/util/Platforms/Win32/Win32Defs.hpp>
#endif

#if defined(XML_CYGWIN)
#include    <xercesc/util/Platforms/Cygwin/CygwinDefs.hpp>
#endif

#if defined(XML_AIX)
#include    <xercesc/util/Platforms/AIX/AIXDefs.hpp>
#endif

#if defined(XML_SOLARIS)
#include    <xercesc/util/Platforms/Solaris/SolarisDefs.hpp>
#endif

#if defined(XML_OPENSERVER)
#include    <xercesc/util/Platforms/OpenServer/OpenServerDefs.hpp>
#endif

#if defined(XML_UNIXWARE)
#include    <xercesc/util/Platforms/UnixWare/UnixWareDefs.hpp>
#endif

#if defined(XML_HPUX)
#include    <xercesc/util/Platforms/HPUX/HPUXDefs.hpp>
#endif

#if defined(XML_IRIX)
#include    <xercesc/util/Platforms/IRIX/IRIXDefs.hpp>
#endif

#if defined(XML_INTERIX)
#include    <xercesc/util/Platforms/Interix/InterixDefs.hpp>
#endif

#if defined(XML_TANDEM)
#include    <xercesc/util/Platforms/Tandem/TandemDefs.hpp>
#endif

#if defined(XML_BEOS)
#include    <xercesc/util/Platforms/BeOS/BeOSDefs.hpp>
#endif

#if defined(XML_LINUX)
#include    <xercesc/util/Platforms/Linux/LinuxDefs.hpp>
#endif

#if defined(XML_FREEBSD)
#include    <xercesc/util/Platforms/FreeBSD/FreeBSDDefs.hpp>
#endif

#if defined(XML_OS390)
#include    <xercesc/util/Platforms/OS390/OS390Defs.hpp>
#endif

#if defined(XML_PTX)
#include    <xercesc/util/Platforms/PTX/PTXDefs.hpp>
#endif

#if defined(XML_OS2)
#include    <xercesc/util/Platforms/OS2/OS2Defs.hpp>
#endif

#if defined(XML_MACOS)
#include	<xercesc/util/Platforms/MacOS/MacOSDefs.hpp>
#endif

#if defined(XML_AS400)
#include	<xercesc/util/Platforms/OS400/OS400Defs.hpp>
#endif

#if defined(XML_TRU64)
#include	<xercesc/util/Platforms/Tru64/Tru64Defs.hpp>
#endif

#if defined(XML_QNX)
#include	<xercesc/util/Platforms/QNX/QNXDefs.hpp>
#endif

// ---------------------------------------------------------------------------
//  And now we subinclude a header according to the development environment
//  we are on. This guy defines for each platform some basic stuff that is
//  specific to the development environment.
// ---------------------------------------------------------------------------
#if defined(XML_VISUALCPP)
#include    <xercesc/util/Compilers/VCPPDefs.hpp>
#endif

#if defined(XML_CSET)
#include    <xercesc/util/Compilers/CSetDefs.hpp>
#endif

#if defined(XML_BORLAND)
#include    <xercesc/util/Compilers/BorlandCDefs.hpp>
#endif

#if defined(XML_SUNCC) || defined(XML_SUNCC5)
#include    <xercesc/util/Compilers/SunCCDefs.hpp>
#endif

#if defined(XML_SCOCC)
#include    <xercesc/util/Compilers/SCOCCDefs.hpp>
#endif

#if defined(XML_SOLARIS_KAICC)
#include    <xercesc/util/Compilers/SunKaiDefs.hpp>
#endif

#if defined(XML_HPUX_CC) || defined(XML_HPUX_aCC) || defined(XML_HPUX_KAICC)
#include    <xercesc/util/Compilers/HPCCDefs.hpp>
#endif

#if defined(XML_MIPSPRO_CC)
#include    <xercesc/util/Compilers/MIPSproDefs.hpp>
#endif

#if defined(XML_TANDEMCC)
#include    <xercesc/util/Compilers/TandemCCDefs.hpp>
#endif

#if defined(XML_GCC)
#include    <xercesc/util/Compilers/GCCDefs.hpp>
#endif

#if defined(XML_MVSCPP)
#include    <xercesc/util/Compilers/MVSCPPDefs.hpp>
#endif

#if defined(XML_IBMVAW32)
#include    <xercesc/util/Compilers/IBMVAW32Defs.hpp>
#endif

#if defined(XML_IBMVAOS2)
#include    <xercesc/util/Compilers/IBMVAOS2Defs.hpp>
#endif

#if defined(XML_METROWERKS)
#include	<xercesc/util/Compilers/CodeWarriorDefs.hpp>
#endif

#if defined(XML_PTX_CC)
#include	<xercesc/util/Compilers/PTXCCDefs.hpp>
#endif

#if defined(XML_AS400)
#include	<xercesc/util/Compilers/OS400SetDefs.hpp>
#endif

#if defined(XML_DECCXX)
#include	<xercesc/util/Compilers/DECCXXDefs.hpp>
#endif

#if defined(XML_QCC)
#include	<xercesc/util/Compilers/QCCDefs.hpp>
#endif

// ---------------------------------------------------------------------------
//  Some general typedefs that are defined for internal flexibility.
//
//  Note  that UTF16Ch is fixed at 16 bits, whereas XMLCh floats in size per
//  platform, to whatever is the native wide char format there. UCS4Ch is
//  fixed at 32 bits. The types we defined them in terms of are defined per
//  compiler, using whatever types are the right ones for them to get these
//  16/32 bit sizes.
//
// ---------------------------------------------------------------------------
typedef unsigned char       XMLByte;
typedef XMLUInt16           UTF16Ch;
typedef XMLUInt32           UCS4Ch;


// ---------------------------------------------------------------------------
//  Handle boolean. If the platform can handle booleans itself, then we
//  map our boolean type to the native type. Otherwise we create a default
//  one as an int and define const values for true and false.
//
//  This flag will be set in the per-development environment stuff above.
// ---------------------------------------------------------------------------
#if defined(NO_NATIVE_BOOL)
  #ifndef bool
    typedef int     bool;
  #endif
  #ifndef true
    #define  true     1
  #endif
  #ifndef false
    #define false 0
  #endif
#endif

#if defined(XML_NETBSD)
#include       <xercesc/util/Platforms/NetBSD/NetBSDDefs.hpp>
#endif

// ---------------------------------------------------------------------------
//  According to whether the compiler suports L"" type strings, we define
//  the XMLStrL() macro one way or another.
// ---------------------------------------------------------------------------
#if defined(XML_LSTRSUPPORT)
#define XMLStrL(str)  L##str
#else
#define XMLStrL(str)  str
#endif


// ---------------------------------------------------------------------------
// Define namespace symbols if the compiler supports it.
// ---------------------------------------------------------------------------
#if defined(XERCES_HAS_CPP_NAMESPACE)
    #define XERCES_CPP_NAMESPACE_BEGIN namespace XERCES_CPP_NAMESPACE {
    #define XERCES_CPP_NAMESPACE_END  }
    #define XERCES_CPP_NAMESPACE_USE using namespace XERCES_CPP_NAMESPACE;
    #define XERCES_CPP_NAMESPACE_QUALIFIER XERCES_CPP_NAMESPACE::

    namespace XERCES_CPP_NAMESPACE { }
    namespace xercesc = XERCES_CPP_NAMESPACE;
#else
    #define XERCES_CPP_NAMESPACE_BEGIN
    #define XERCES_CPP_NAMESPACE_END
    #define XERCES_CPP_NAMESPACE_USE
    #define XERCES_CPP_NAMESPACE_QUALIFIER
#endif

#if defined(XERCES_STD_NAMESPACE)
	#define XERCES_USING_STD(NAME) using std :: NAME;
	#define XERCES_STD_QUALIFIER  std ::
#else
	#define XERCES_USING_STD(NAME)
	#define XERCES_STD_QUALIFIER 
#endif


// ---------------------------------------------------------------------------
//  Set up the import/export keyword  for our core projects. The
//  PLATFORM_XXXX keywords are set in the per-development environment
//  include above.
// ---------------------------------------------------------------------------
#if defined(PROJ_XMLUTIL)
#define XMLUTIL_EXPORT PLATFORM_EXPORT
#else
#define XMLUTIL_EXPORT PLATFORM_IMPORT
#endif

#if defined(PROJ_XMLPARSER)
#define XMLPARSER_EXPORT PLATFORM_EXPORT
#else
#define XMLPARSER_EXPORT PLATFORM_IMPORT
#endif

#if defined(PROJ_SAX4C)
#define SAX_EXPORT PLATFORM_EXPORT
#else
#define SAX_EXPORT PLATFORM_IMPORT
#endif

#if defined(PROJ_SAX2)
#define SAX2_EXPORT PLATFORM_EXPORT
#else
#define SAX2_EXPORT PLATFORM_IMPORT
#endif

#if defined(PROJ_DOM)
#define CDOM_EXPORT PLATFORM_EXPORT
#else
#define CDOM_EXPORT PLATFORM_IMPORT
#endif

#if defined(PROJ_DEPRECATED_DOM)
#define DEPRECATED_DOM_EXPORT PLATFORM_EXPORT
#else
#define DEPRECATED_DOM_EXPORT PLATFORM_IMPORT
#endif

#if defined(PROJ_PARSERS)
#define PARSERS_EXPORT  PLATFORM_EXPORT
#else
#define PARSERS_EXPORT  PLATFORM_IMPORT
#endif

#if defined(PROJ_VALIDATORS)
#define VALIDATORS_EXPORT  PLATFORM_EXPORT
#else
#define VALIDATORS_EXPORT  PLATFORM_IMPORT
#endif

#endif
