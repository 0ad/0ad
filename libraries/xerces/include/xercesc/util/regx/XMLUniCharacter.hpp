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
 * $Id: XMLUniCharacter.hpp,v 1.2 2002/11/04 15:17:01 tng Exp $
 */

#if !defined(XMLUNICHARACTER_HPP)
#define XMLUNICHARACTER_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
  * Class for representing unicode characters
  */
class XMLUTIL_EXPORT XMLUniCharacter
{
public:
    // -----------------------------------------------------------------------
    //  Public Constants
    // -----------------------------------------------------------------------
    // Unicode chara types
    enum {
        UNASSIGNED              = 0,
        UPPERCASE_LETTER        = 1,
        LOWERCASE_LETTER        = 2,
        TITLECASE_LETTER        = 3,
        MODIFIER_LETTER         = 4,
        OTHER_LETTER            = 5,
        NON_SPACING_MARK        = 6,
        ENCLOSING_MARK          = 7,
        COMBINING_SPACING_MARK  = 8,
        DECIMAL_DIGIT_NUMBER    = 9,
        LETTER_NUMBER           = 10,
        OTHER_NUMBER            = 11,
        SPACE_SEPARATOR         = 12,
        LINE_SEPARATOR          = 13,
        PARAGRAPH_SEPARATOR     = 14,
        CONTROL                 = 15,
        FORMAT                  = 16,
        PRIVATE_USE             = 17,
        SURROGATE               = 18,
        DASH_PUNCTUATION        = 19,
        START_PUNCTUATION       = 20,
        END_PUNCTUATION         = 21,
		CONNECTOR_PUNCTUATION   = 22,
        OTHER_PUNCTUATION       = 23,
        MATH_SYMBOL             = 24,
        CURRENCY_SYMBOL         = 25,
        MODIFIER_SYMBOL         = 26,
        OTHER_SYMBOL            = 27,
		INITIAL_PUNCTUATION     = 28,
		FINAL_PUNCTUATION       = 29
	};

	/** destructor */
    ~XMLUniCharacter() {}

    /* Static methods for getting unicode character type */
    /** @name Getter functions */
    //@{

    /** Gets the unicode type of a given character
      *
      * @param ch The character we want to get its unicode type
      */
    static unsigned short getType(const XMLCh ch);
	//@}

private :

    /** @name Constructors and Destructor */
    //@{
    /** Unimplemented default constructor */
    XMLUniCharacter();
    //@}
};

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file XMLUniCharacter.hpp
  */
