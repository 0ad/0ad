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
 *  $Id: XMLRecognizer.hpp 176026 2004-09-08 13:57:07Z peiyongz $
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
