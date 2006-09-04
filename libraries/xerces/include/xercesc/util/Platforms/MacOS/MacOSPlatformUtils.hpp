/*
 * Copyright 1999-2000,2004 The Apache Software Foundation.
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
 * $Id: MacOSPlatformUtils.hpp 180016 2005-06-04 19:49:30Z jberry $
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
XMLUTIL_EXPORT XMLMacAbstractFile* XMLMakeMacFile(MemoryManager* manager);

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
extern bool gMacOSXOrBetter;
extern bool gHasFSSpecAPIs;
extern bool gHasFS2TBAPIs;
extern bool gHasHFSPlusAPIs;
extern bool gHasFSPathAPIs;
extern bool gPathAPIsUsePosixPaths;
extern bool gHasMPAPIs;
extern bool gUsePosixFiles;

XERCES_CPP_NAMESPACE_END

