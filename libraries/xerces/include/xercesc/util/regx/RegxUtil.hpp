/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 2001, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: RegxUtil.hpp,v 1.4 2003/05/16 00:03:10 knoaman Exp $
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
