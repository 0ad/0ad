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
 *  $Id: XMLRecognizer.hpp,v 1.6 2004/01/29 11:46:29 cargilld Exp $
 */

#if !defined(XMLRECOGNIZER_HPP)
#define XMLRECOGNIZER_HPP

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 *  This class provides some simple code to recognize the encodings of
 *  XML files. This recognition only does very basic sensing of the encoding
 *  in a broad sense. Basically its just enough to let us get started and
 *  read the XMLDecl line. The scanner, once it reads the XMLDecl, will
 *  tell the reader any actual encoding string it found and the reader can
 *  update itself to be more specific at that point.
 */
class XMLPARSER_EXPORT XMLRecognizer
{
public :
    // -----------------------------------------------------------------------
    //  Class types
    //
    //  This enum represents the various encoding families that we have to
    //  deal with individually at the scanner level. This does not indicate
    //  the exact encoding, just the rough family that would let us scan
    //  the XML/TextDecl to find the encoding string.
    //
    //  The 'L's and 'B's stand for little or big endian. We conditionally
    //  create versions that will automatically map to the local UTF-16 and
    //  UCS-4 endian modes.
    //
    //  OtherEncoding means that its some transcoder based encoding, i.e. not
    //  one of the ones that we do internally. Its a special case and should
    //  never be used directly outside of the reader.
    //
    //  NOTE: Keep this in sync with the name map array in the Cpp file!!
    // -----------------------------------------------------------------------
    enum Encodings
    {
        EBCDIC          = 0
        , UCS_4B        = 1
        , UCS_4L        = 2
        , US_ASCII      = 3
        , UTF_8         = 4
        , UTF_16B       = 5
        , UTF_16L       = 6
        , XERCES_XMLCH  = 7

        , Encodings_Count
        , Encodings_Min = EBCDIC
        , Encodings_Max = XERCES_XMLCH

        , OtherEncoding = 999

        #if defined(ENDIANMODE_BIG)
        , Def_UTF16     = UTF_16B
        , Def_UCS4      = UCS_4B
        #else
        , Def_UTF16     = UTF_16L
        , Def_UCS4      = UCS_4L
        #endif
    };


    // -----------------------------------------------------------------------
    //  Public, const static data
    //
    //  These are the byte sequences for each of the encodings that we can
    //  auto sense, and their lengths.
    // -----------------------------------------------------------------------
    static const char           fgASCIIPre[];
    static const unsigned int   fgASCIIPreLen;
    static const XMLByte        fgEBCDICPre[];
    static const unsigned int   fgEBCDICPreLen;
    static const XMLByte        fgUTF16BPre[];
    static const XMLByte        fgUTF16LPre[];
    static const unsigned int   fgUTF16PreLen;
    static const XMLByte        fgUCS4BPre[];
    static const XMLByte        fgUCS4LPre[];
    static const unsigned int   fgUCS4PreLen;
    static const char           fgUTF8BOM[];
    static const unsigned int   fgUTF8BOMLen;


    // -----------------------------------------------------------------------
    //  Encoding recognition methods
    // -----------------------------------------------------------------------
    static Encodings basicEncodingProbe
    (
        const   XMLByte* const      rawBuffer
        , const unsigned int        rawByteCount
    );

    static Encodings encodingForName
    (
        const   XMLCh* const    theEncName
    );

    static const XMLCh* nameForEncoding(const Encodings theEncoding
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);


protected :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors, operators, and destructor
    //
    //  This class is effectively being used as a namespace for some static
    //  methods.
    //
    //   (these functions are protected rather than private only to get rid of
    //    some annoying compiler warnings.)
    //
    // -----------------------------------------------------------------------
    XMLRecognizer();
    ~XMLRecognizer();

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLRecognizer(const XMLRecognizer&);    
    XMLRecognizer& operator=(const XMLRecognizer&);
};

XERCES_CPP_NAMESPACE_END

#endif
