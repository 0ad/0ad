/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002 The Apache Software Foundation.  All rights
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
 * $Log: XMLChar.hpp,v $
 * Revision 1.3  2004/01/29 11:48:47  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.2  2003/08/14 02:57:27  knoaman
 * Code refactoring to improve performance of validation.
 *
 * Revision 1.1  2002/12/20 22:10:21  tng
 * XML 1.1
 *
 */

#if !defined(XMLCHAR_HPP)
#define XMLCHAR_HPP

#include <xercesc/util/XMLUniDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  This file defines Char and utility that conforms to XML 1.0 and XML 1.1
// ---------------------------------------------------------------------------
// Masks for the fgCharCharsTable1_0 array
const XMLByte   gLetterCharMask             = 0x1;
const XMLByte   gFirstNameCharMask          = 0x2;
const XMLByte   gNameCharMask               = 0x4;
const XMLByte   gPlainContentCharMask       = 0x8;
const XMLByte   gSpecialStartTagCharMask    = 0x10;
const XMLByte   gControlCharMask            = 0x20;
const XMLByte   gXMLCharMask                = 0x40;
const XMLByte   gWhitespaceCharMask         = 0x80;

// ---------------------------------------------------------------------------
//  This class is for XML 1.0
// ---------------------------------------------------------------------------
class XMLUTIL_EXPORT XMLChar1_0
{
public:
    // -----------------------------------------------------------------------
    //  Public, static methods, check the string
    // -----------------------------------------------------------------------
    static bool isAllSpaces
    (
        const   XMLCh* const    toCheck
        , const unsigned int    count
    );

    static bool containsWhiteSpace
    (
        const   XMLCh* const    toCheck
        , const unsigned int    count
    );

    static bool isValidName
    (
        const   XMLCh* const    toCheck
        , const unsigned int    count
    );

    static bool isValidNCName
    (
        const   XMLCh* const    toCheck
        , const unsigned int    count
    );

    static bool isValidQName
    (
        const   XMLCh* const    toCheck
        , const unsigned int    count
    );

    // -----------------------------------------------------------------------
    //  Public, static methods, check the XMLCh
    //  surrogate pair is assumed if second parameter is not null
    // -----------------------------------------------------------------------
    static bool isXMLLetter(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isFirstNameChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isNameChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isPlainContentChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isSpecialStartTagChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isXMLChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isWhitespace(const XMLCh toCheck);
    static bool isWhitespace(const XMLCh toCheck, const XMLCh toCheck2);
    static bool isControlChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);

    static bool isPublicIdChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);

    // -----------------------------------------------------------------------
    //  Special Non-conformant Public, static methods
    // -----------------------------------------------------------------------
    /**
      * Return true if NEL (0x85) and LSEP (0x2028) to be treated as white space char.
      */
    static bool isNELRecognized();

    /**
      * Method to enable NEL (0x85) and LSEP (0x2028) to be treated as white space char.
      */
    static void enableNELWS();

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLChar1_0();

    // -----------------------------------------------------------------------
    //  Static data members
    //
    //  fgCharCharsTable1_0
    //      The character characteristics table. Bits in each byte, represent
    //      the characteristics of each character. It is generated via some
    //      code and then hard coded into the cpp file for speed.
    //
    //  fNEL
    //      Flag to respresents whether NEL and LSEP newline recognition is enabled
    //      or disabled
    // -----------------------------------------------------------------------
    static XMLByte  fgCharCharsTable1_0[0x10000];
    static bool     enableNEL;

    friend class XMLReader;
};


// ---------------------------------------------------------------------------
//  XMLReader: Public, static methods
// ---------------------------------------------------------------------------
inline bool XMLChar1_0::isXMLLetter(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_0[toCheck] & gLetterCharMask) != 0);
    return false;
}

inline bool XMLChar1_0::isFirstNameChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_0[toCheck] & gFirstNameCharMask) != 0);
    return false;
}

inline bool XMLChar1_0::isNameChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_0[toCheck] & gNameCharMask) != 0);
    return false;
}

inline bool XMLChar1_0::isPlainContentChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_0[toCheck] & gPlainContentCharMask) != 0);
    else {
        if ((toCheck >= 0xD800) && (toCheck <= 0xDBFF))
           if ((toCheck2 >= 0xDC00) && (toCheck2 <= 0xDFFF))
               return true;
    }
    return false;
}


inline bool XMLChar1_0::isSpecialStartTagChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_0[toCheck] & gSpecialStartTagCharMask) != 0);
    return false;
}

inline bool XMLChar1_0::isXMLChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_0[toCheck] & gXMLCharMask) != 0);
    else {
        if ((toCheck >= 0xD800) && (toCheck <= 0xDBFF))
           if ((toCheck2 >= 0xDC00) && (toCheck2 <= 0xDFFF))
               return true;
    }
    return false;
}

inline bool XMLChar1_0::isWhitespace(const XMLCh toCheck)
{
    return ((fgCharCharsTable1_0[toCheck] & gWhitespaceCharMask) != 0);
}

inline bool XMLChar1_0::isWhitespace(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_0[toCheck] & gWhitespaceCharMask) != 0);
    return false;
}

inline bool XMLChar1_0::isControlChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_0[toCheck] & gControlCharMask) != 0);
    return false;
}

inline bool XMLChar1_0::isNELRecognized() {

    return enableNEL;
}


// ---------------------------------------------------------------------------
//  This class is for XML 1.1
// ---------------------------------------------------------------------------
class XMLUTIL_EXPORT XMLChar1_1
{
public:
    // -----------------------------------------------------------------------
    //  Public, static methods, check the string
    // -----------------------------------------------------------------------
    static bool isAllSpaces
    (
        const   XMLCh* const    toCheck
        , const unsigned int    count
    );

    static bool containsWhiteSpace
    (
        const   XMLCh* const    toCheck
        , const unsigned int    count
    );

    static bool isValidName
    (
        const   XMLCh* const    toCheck
        , const unsigned int    count
    );

    static bool isValidNCName
    (
        const   XMLCh* const    toCheck
        , const unsigned int    count
    );

    static bool isValidQName
    (
        const   XMLCh* const    toCheck
        , const unsigned int    count
    );

    // -----------------------------------------------------------------------
    //  Public, static methods, check the XMLCh
    // -----------------------------------------------------------------------
    static bool isXMLLetter(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isFirstNameChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isNameChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isPlainContentChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isSpecialStartTagChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isXMLChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isWhitespace(const XMLCh toCheck, const XMLCh toCheck2 = 0);
    static bool isControlChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);

    static bool isPublicIdChar(const XMLCh toCheck, const XMLCh toCheck2 = 0);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLChar1_1();

    // -----------------------------------------------------------------------
    //  Static data members
    //
    //  fgCharCharsTable1_1
    //      The character characteristics table. Bits in each byte, represent
    //      the characteristics of each character. It is generated via some
    //      code and then hard coded into the cpp file for speed.
    //
    // -----------------------------------------------------------------------
    static XMLByte  fgCharCharsTable1_1[0x10000];

    friend class XMLReader;
};


// ---------------------------------------------------------------------------
//  XMLReader: Public, static methods
// ---------------------------------------------------------------------------
inline bool XMLChar1_1::isXMLLetter(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_1[toCheck] & gLetterCharMask) != 0);
    return false;
}

inline bool XMLChar1_1::isFirstNameChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_1[toCheck] & gFirstNameCharMask) != 0);
    else {
        if ((toCheck >= 0xD800) && (toCheck <= 0xDB7F))
           if ((toCheck2 >= 0xDC00) && (toCheck2 <= 0xDFFF))
               return true;
    }
    return false;
}

inline bool XMLChar1_1::isNameChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_1[toCheck] & gNameCharMask) != 0);
    else {
        if ((toCheck >= 0xD800) && (toCheck <= 0xDB7F))
           if ((toCheck2 >= 0xDC00) && (toCheck2 <= 0xDFFF))
               return true;
    }
    return false;
}

inline bool XMLChar1_1::isPlainContentChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_1[toCheck] & gPlainContentCharMask) != 0);
    else {
        if ((toCheck >= 0xD800) && (toCheck <= 0xDBFF))
           if ((toCheck2 >= 0xDC00) && (toCheck2 <= 0xDFFF))
               return true;
    }
    return false;
}


inline bool XMLChar1_1::isSpecialStartTagChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_1[toCheck] & gSpecialStartTagCharMask) != 0);
    return false;
}

inline bool XMLChar1_1::isXMLChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_1[toCheck] & gXMLCharMask) != 0);
    else {
        if ((toCheck >= 0xD800) && (toCheck <= 0xDBFF))
           if ((toCheck2 >= 0xDC00) && (toCheck2 <= 0xDFFF))
               return true;
    }
    return false;
}

inline bool XMLChar1_1::isWhitespace(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_1[toCheck] & gWhitespaceCharMask) != 0);
    return false;
}

inline bool XMLChar1_1::isControlChar(const XMLCh toCheck, const XMLCh toCheck2)
{
    if (!toCheck2)
        return ((fgCharCharsTable1_1[toCheck] & gControlCharMask) != 0);
    return false;
}


XERCES_CPP_NAMESPACE_END

#endif
