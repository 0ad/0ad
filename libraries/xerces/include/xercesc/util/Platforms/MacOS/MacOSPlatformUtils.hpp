/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2000 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xerces" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache\@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation, and was
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: MacOSPlatformUtils.hpp,v 1.11 2003/08/27 16:41:56 jberry Exp $
 */

#pragma once

#include <cstdlib>
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/Platforms/MacOS/MacAbstractFile.hpp>


#if defined(__APPLE__)
    //	Framework includes from ProjectBuilder
    #include <CoreServices/CoreServices.h>
#else
    //	Classic includes otherwise
    #include <Files.h>
#endif

XERCES_CPP_NAMESPACE_BEGIN

//	Notes on our Xerces/Mac paths:
//
//	Wherever paths are used in Xerces, this Macintosh port assumes that they'll
//	be in "unix" format, or at least as close as you can get to that on the particular
//	OS. On classic, this means that a path will be a unix style path, separated by '/' and
//	starting with the Mac OS volume name. Since slash is used as the segment separator,
//	any slashes that actually exist in the segment name will be converted to colons
//	(since colon is the Mac OS path separator and would be illegal in a segment name).
//	For Mac OS X, paths are created and parsed using the FSRefMakePath, etc, routines:
//	the major difference will be location of the volume name within the path.
//
//	The routines below help to create and interpret these pathnames for these cases.
//	While the port itself never creates such paths, it does use these same routines to
//	parse them.

//	Factory method to create an appropriate concrete object
//	descended from XMLMacAbstractFile.
XMLUTIL_EXPORT XMLMacAbstractFile* XMLMakeMacFile(void);

//	Convert fom FSRef/FSSpec to a Unicode character string path.
//	Note that you'll need to delete [] that string after you're done with it!
XMLUTIL_EXPORT XMLCh*	XMLCreateFullPathFromFSRef(const FSRef& startingRef,
                            MemoryManager* const manager = XMLPlatformUtils::fgArrayMemoryManager);
XMLUTIL_EXPORT XMLCh*	XMLCreateFullPathFromFSSpec(const FSSpec& startingSpec,
                            MemoryManager* const manager = XMLPlatformUtils::fgArrayMemoryManager);

//	Convert from path to FSRef/FSSpec
//	You retain ownership of the pathName.
//	Note: in the general case, these routines will fail if the specified file
//	      does not exist when the routine is called.
XMLUTIL_EXPORT bool	XMLParsePathToFSRef(const XMLCh* const pathName, FSRef& ref,
                            MemoryManager* const manager = XMLPlatformUtils::fgArrayMemoryManager);
XMLUTIL_EXPORT bool	XMLParsePathToFSSpec(const XMLCh* const pathName, FSSpec& spec,
                            MemoryManager* const manager = XMLPlatformUtils::fgArrayMemoryManager);

//	These routines copy characters between their representation in the Unicode Converter
//	and the representation used by XMLCh. Until a recent change in Xerces, these were
//	sometimes different on the Macintosh (with GCC), but XMLCh is now fixed at 16 bits.
//	Code utilitizing these routines may be phased out in time, as a conversion is no
//	longer necessary.
XMLUTIL_EXPORT XMLCh*
CopyUniCharsToXMLChs(const UniChar* src, XMLCh* dst, std::size_t charCount, std::size_t maxChars);
XMLUTIL_EXPORT UniChar*
CopyXMLChsToUniChars(const XMLCh* src, UniChar* dst, std::size_t charCount, std::size_t maxChars);

//	UTF8/UniChar transcoding utilities
XMLUTIL_EXPORT std::size_t
TranscodeUniCharsToUTF8(const UniChar* src, char* dst, std::size_t srcCnt, std::size_t maxChars);
XMLUTIL_EXPORT std::size_t
TranscodeUTF8ToUniChars(const char* src, UniChar* dst, std::size_t maxChars);

// Size of our statically allocated path buffers
const std::size_t kMaxMacStaticPathChars = 512;

//	Global variables set in platformInit()
extern bool gFileSystemCompatible;
extern bool gHasFSSpecAPIs;
extern bool gHasFS2TBAPIs;
extern bool gHasHFSPlusAPIs;
extern bool gHasFSPathAPIs;
extern bool gPathAPIsUsePosixPaths;
extern bool gHasMPAPIs;
extern bool gUsePosixFiles;

XERCES_CPP_NAMESPACE_END

