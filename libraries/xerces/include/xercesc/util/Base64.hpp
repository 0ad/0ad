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
 * $Id: Base64.hpp,v 1.8 2003/12/17 00:18:35 cargilld Exp $
 */

#ifndef BASE64_HPP
#define BASE64_HPP

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/framework/MemoryManager.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
// This class provides encode/decode for RFC 2045 Base64 as
// defined by RFC 2045, N. Freed and N. Borenstein.
// RFC 2045: Multipurpose Internet Mail Extensions (MIME)
// Part One: Format of Internet Message Bodies. Reference
// 1996 Available at: http://www.ietf.org/rfc/rfc2045.txt
// This class is used by XML Schema binary format validation
//
//
class XMLUTIL_EXPORT Base64
{
public :

    //@{

    /**
     * Encodes octets into Base64 data
     *
     * NOTE: The returned buffer is dynamically allocated and is the
     * responsibility of the caller to delete it when not longer needed.
     * You can call XMLString::release to release this returned buffer.
     *
     * If a memory manager is provided, ask the memory manager to de-allocate
     * the returned buffer.
     *
     * @param inputData Binary data in XMLByte stream.
     * @param inputLength Length of the XMLByte stream.
     * @param outputLength Length of the encoded Base64 byte stream.
     * @param memMgr client provided memory manager
     * @return Encoded Base64 data in XMLByte stream,
     *      or NULL if input data can not be encoded.
     * @see   XMLString::release(XMLByte**)
     */
    static XMLByte* encode(const XMLByte* const inputData
                         , const unsigned int   inputLength
                         , unsigned int*        outputLength                         
                         , MemoryManager* const memMgr = 0);

    /**
     * Decodes Base64 data into octets
     *
     * NOTE: The returned buffer is dynamically allocated and is the
     * responsibility of the caller to delete it when not longer needed.
     * You can call XMLString::release to release this returned buffer.
     *
     * If a memory manager is provided, ask the memory manager to de-allocate
     * the returned buffer.
     *
     * @param inputData Base64 data in XMLByte stream.
     * @param outputLength Length of decoded XMLByte stream.
     * @param memMgr client provided memory manager
     * @return Decoded binary data in XMLByte stream,
     *      or NULL if input data can not be decoded.
     * @see   XMLString::release(XMLByte**)
     */
    static XMLByte* decode(const XMLByte* const inputData
                         , unsigned int*        outputLength                         
                         , MemoryManager* const memMgr = 0);

    /**
     * Decodes Base64 data into XMLCh
     *
     * NOTE: The returned buffer is dynamically allocated and is the
     * responsibility of the caller to delete it when not longer needed.
     * You can call XMLString::release to release this returned buffer.
     *
     * If a memory manager is provided, ask the memory manager to de-allocate
     * the returned buffer.
     *
     * @param inputData Base64 data in XMLCh stream.
     * @param outputLength Length of decoded XMLCh stream
     * @param memMgr client provided memory manager
     * @return Decoded binary data in XMLCh stream,
     *      or NULL if input data can not be decoded.
     * @see   XMLString::release(XMLCh**)
     */
    static XMLCh* decode(const XMLCh* const   inputData
                       , unsigned int*        outputLength
                       , MemoryManager* const memMgr = 0);

    /**
     * Get data length
	 *
     * Returns length of decoded data given an array
     * containing encoded data.
     *
     * @param inputData Base64 data in XMLCh stream.
     * @return Length of decoded data,
	 *      or -1 if input data can not be decoded.
     */
    static int getDataLength(const XMLCh* const inputData
        , MemoryManager* const memMgr = 0);

    //@}

private :

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------

    static void init();

    static bool isData(const XMLByte& octet);
    static bool isPad(const XMLByte& octet);

    static XMLByte set1stOctet(const XMLByte&, const XMLByte&);
    static XMLByte set2ndOctet(const XMLByte&, const XMLByte&);
    static XMLByte set3rdOctet(const XMLByte&, const XMLByte&);

    static void split1stOctet(const XMLByte&, XMLByte&, XMLByte&);
    static void split2ndOctet(const XMLByte&, XMLByte&, XMLByte&);
    static void split3rdOctet(const XMLByte&, XMLByte&, XMLByte&);

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    Base64();
    Base64(const Base64&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  base64Alphabet
    //     The Base64 alphabet (see RFC 2045).
    //
    //  base64Padding
    //     Padding character (see RFC 2045).
    //
    //  base64Inverse
    //     Table used in decoding base64.
    //
    //  isInitialized
    //     Set once base64Inverse is initalized.
    //
    //  quadsPerLine
    //     Number of quadruplets per one line. The encoded output
    //     stream must be represented in lines of no more
    //     than 19 quadruplets each.
    //
    // -----------------------------------------------------------------------

    static const XMLByte  base64Alphabet[];
    static const XMLByte  base64Padding;

    static XMLByte  base64Inverse[];
    static bool  isInitialized;

    static const unsigned int  quadsPerLine;
};

// -----------------------------------------------------------------------
//  Helper methods
// -----------------------------------------------------------------------
inline bool Base64::isPad(const XMLByte& octet)
{
    return ( octet == base64Padding );
}

inline XMLByte Base64::set1stOctet(const XMLByte& b1, const XMLByte& b2)
{
    return (( b1 << 2 ) | ( b2 >> 4 ));
}

inline XMLByte Base64::set2ndOctet(const XMLByte& b2, const XMLByte& b3)
{
    return (( b2 << 4 ) | ( b3 >> 2 ));
}

inline XMLByte Base64::set3rdOctet(const XMLByte& b3, const XMLByte& b4)
{
    return (( b3 << 6 ) | b4 );
}

inline void Base64::split1stOctet(const XMLByte& ch, XMLByte& b1, XMLByte& b2) {
    b1 = ch >> 2;
    b2 = ( ch & 0x3 ) << 4;
}

inline void Base64::split2ndOctet(const XMLByte& ch, XMLByte& b2, XMLByte& b3) {
    b2 |= ch >> 4;  // combine with previous value
    b3 = ( ch & 0xf ) << 2;
}

inline void Base64::split3rdOctet(const XMLByte& ch, XMLByte& b3, XMLByte& b4) {
    b3 |= ch >> 6;  // combine with previous value
    b4 = ( ch & 0x3f );
}

XERCES_CPP_NAMESPACE_END

#endif
