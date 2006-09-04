/*
 * Copyright 2001,2004 The Apache Software Foundation.
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
 * $Id: RegxUtil.hpp 225473 2005-07-27 07:09:16Z dbertoni $
 */

#if !defined(REGXUTIL_HPP)
#define REGXUTIL_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/XMLUniDefs.hpp>


XERCES_CPP_NAMESPACE_BEGIN

class MemoryManager;

class XMLUTIL_EXPORT RegxUtil {
public:

	// -----------------------------------------------------------------------
    //  Constructors and destructors
    // -----------------------------------------------------------------------
	~RegxUtil() {}

	static XMLInt32 composeFromSurrogate(const XMLCh high, const XMLCh low);
	static bool isEOLChar(const XMLCh);
	static bool isWordChar(const XMLCh);
	static bool isLowSurrogate(const XMLCh ch);
	static bool isHighSurrogate(const XMLCh ch);
	static void decomposeToSurrogates(XMLInt32 ch, XMLCh& high, XMLCh& low);

	static XMLCh* decomposeToSurrogates(XMLInt32 ch,
                                        MemoryManager* const manager);
	static XMLCh* stripExtendedComment(const XMLCh* const expression,
                                       MemoryManager* const manager = 0);

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RegxUtil();
};


inline bool RegxUtil::isEOLChar(const XMLCh ch) {

	return (ch == chLF || ch == chCR || ch == chLineSeparator
           || ch == chParagraphSeparator);
}

inline XMLInt32 RegxUtil::composeFromSurrogate(const XMLCh high, const XMLCh low) {

	return 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
}

inline bool RegxUtil::isLowSurrogate(const XMLCh ch) {

	return (ch & 0xFC00) == 0xDC00;
}

inline bool RegxUtil::isHighSurrogate(const XMLCh ch) {

	return (ch & 0xFC00) == 0xD800;
}

inline void RegxUtil::decomposeToSurrogates(XMLInt32 ch, XMLCh& high, XMLCh& low) {

    ch -= 0x10000;
	high = XMLCh((ch >> 10) + 0xD800);
	low = XMLCh((ch & 0x03FF) + 0xDC00);
}

inline bool RegxUtil::isWordChar(const XMLCh ch) {

	if ((ch == chUnderscore)
		|| (ch >= chDigit_0 && ch <= chDigit_9)
		|| (ch >= chLatin_A && ch <= chLatin_Z)
		|| (ch >= chLatin_a && ch <= chLatin_z))
		return true;

	return false;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file RegxUtil.hpp
  */
