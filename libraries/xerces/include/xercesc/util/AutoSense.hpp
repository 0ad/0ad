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
 * $Id: AutoSense.hpp 191054 2005-06-17 02:56:35Z jberry $
 */

#ifndef AUTOSENSE_HPP
#define AUTOSENSE_HPP

// ---------------------------------------------------------------------------
//  This section attempts to auto detect the operating system. It will set
//  up XercesC specific defines that are used by the rest of the code.
// ---------------------------------------------------------------------------
#if defined(_AIX)
    #define XML_AIX
    #define XML_UNIX
    #if defined(_AIXVERSION_430)
        #define XML_AIX43  // for use of POSIX compliant pthread functions
    #endif
#elif defined(_SEQUENT_)
    #define XML_PTX
    #define XML_UNIX
#elif defined(_HP_UX) || defined(__hpux) || defined(_HPUX_SOURCE)
    #define XML_HPUX
    #define XML_UNIX
#elif defined(SOLARIS) || defined(__SVR4)
    #define XML_SOLARIS
    #define XML_UNIX
#elif defined(_SCO_DS)
    #define XML_OPENSERVER
    #define XML_UNIX
#elif defined(__UNIXWARE__) || defined(__USLC__)
    #define XML_UNIXWARE
    #define XML_UNIX
#elif defined(__BEOS__)
    #define XML_BEOS
    #define XML_UNIX
#elif defined(__QNXNTO__)
    #define XML_QNX
    #define XML_UNIX
#elif defined(__linux__)
    #define XML_LINUX
    #define XML_UNIX
    #if defined(__s390__)
        #define XML_LINUX_390
    #endif
#elif defined(__FreeBSD__)
    #define XML_FREEBSD
    #define XML_UNIX
#elif defined(IRIX) || defined(__sgi)
    #define XML_IRIX
    #define XML_UNIX
#elif defined(__MVS__)
    #define XML_OS390
    #define XML_UNIX
#elif defined(EXM_OS390)
    #define XML_OS390
    #define XML_UNIX
#elif defined(__OS400__)
    #define XML_AS400
    #define XML_UNIX
#elif defined(__OS2__)
    #define XML_OS2
#elif defined(__TANDEM)
    #define XML_TANDEM
    #define XML_UNIX
    #define XML_CSET
#elif defined(__CYGWIN__)
    #define XML_CYGWIN
    #ifndef WIN32
      #define WIN32
    #endif
#elif defined(_WIN32) || defined(WIN32)
    #define XML_WIN32
    #ifndef WIN32
      #define WIN32
    #endif
#elif defined(__WINDOWS__)

    // IBM VisualAge special handling
    #if defined(__32BIT__)
        #define XML_WIN32
    #else
        #define XML_WIN16
    #endif
#elif defined(__MSDXML__)
    #define XML_DOS

#elif defined(macintosh) || (defined(__APPLE__) && defined(__MACH__))
    #define XML_MACOS
#elif defined(__alpha) && defined(__osf__)
    #define XML_TRU64
#elif defined(__NetBSD__)
    #define XML_NETBSD
#elif defined(__INTERIX)
    #define XML_INTERIX
    #define XML_UNIX
#else
    #error Code requires port to host OS!
#endif


// ---------------------------------------------------------------------------
//  This section attempts to autodetect the compiler being used. It will set
//  up Xerces specific defines that can be used by the rest of the code.
// ---------------------------------------------------------------------------
#if defined(__BORLANDC__)
    #define XML_BORLAND
#elif defined(_MSC_VER)
    #define XML_VISUALCPP
#elif defined(XML_SOLARIS)
    #if defined(__SUNPRO_CC) && (__SUNPRO_CC >=0x500)
        #define XML_SUNCC5
	#elif defined(__SUNPRO_CC) && (__SUNPRO_CC <0x500)
        #define XML_SUNCC
    #elif defined(_EDG_RUNTIME_USES_NAMESPACES)
        #define XML_SOLARIS_KAICC
    #elif defined(__GNUG__)
		#define XML_GCC
    #else
        #error Code requires port to current development environment
    #endif
#elif defined (__QNXNTO__)
    #define XML_QCC
#elif defined(__IBMC__) || defined(__IBMCPP__)
    #if defined(XML_WIN32)
        #define XML_IBMVAW32
    #elif defined(XML_OS2)
        #define XML_IBMVAOS2
        #if (__IBMC__ >= 400 || __IBMCPP__ >= 400)
            #define XML_IBMVA4_OS2
        #endif
    #elif defined(XML_AIX) || defined(__linux__)
        #define XML_CSET              
    #elif defined(__MVS__) && defined(__cplusplus)
        #define XML_MVSCPP
    #elif defined(EXM_OS390) && defined(__cplusplus)
        #define XML_MVSCPP
    #endif
#elif defined (__GNUG__) || defined(__BEOS__) || defined(__linux__) || defined(__CYGWIN__)
    #define XML_GCC
#elif defined(XML_HPUX)
    #if defined(EXM_HPUX)
        #define XML_HPUX_KAICC
    #elif (__cplusplus == 1)
        #define XML_HPUX_CC
    #elif (__cplusplus == 199707 || __cplusplus == 199711)
        #define XML_HPUX_aCC
    #endif
#elif defined(XML_IRIX)
    #define XML_MIPSPRO_CC
#elif defined(XML_PTX)
    #define XML_PTX_CC
#elif defined(XML_TANDEM)
    #define XML_TANDEMCC
#elif defined(__MVS__) && defined(__cplusplus)
    #define XML_MVSCPP
#elif defined(EXM_OS390) && defined(__cplusplus)
    #define XML_MVSCPP
#elif defined(XML_TRU64) && defined(__DECCXX)
    #define XML_DECCXX
#elif defined(__MWERKS__)
    #define XML_METROWERKS
#elif defined(__OS400__)
#elif defined(XML_UNIXWARE)
    #define XML_SCOCC
#else
    #error Code requires port to current development environment
#endif

// ---------------------------------------------------------------------------
//  The gcc compiler 2.95... is generating an internal error for some template
//  definitions. So, if we are compiling with gcc, have a specific define that
//  we can later use in the code.
// ---------------------------------------------------------------------------
#if defined(__GNUC__)
#define XML_GCC_VERSION (__GNUC__ * 10000 \
                         + __GNUC_MINOR__ * 100 \
                         + __GNUC_PATCHLEVEL__)
#endif


#endif
