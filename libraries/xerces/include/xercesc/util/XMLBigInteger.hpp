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
 * originally based on software copyright (c) 2001 International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: XMLBigInteger.hpp,v 1.12 2004/01/29 11:48:46 cargilld Exp $
 */

#ifndef XML_BIGINTEGER_HPP
#define XML_BIGINTEGER_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/XMLString.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT XMLBigInteger : public XMemory
{
public:

    /**
     * Constructs a newly allocated <code>XMLBigInteger</code> object that
     * represents the value represented by the string. The string is
     * converted to an int value as if by the <code>valueOf</code> method.
     *
     * @param      strValue   the <code>String</code> to be converted to an
     *                       <code>XMLBigInteger</code>.
     * @param manager    Pointer to the memory manager to be used to
     *                   allocate objects.
     * @exception  NumberFormatException  if the <code>String</code> does not
     *               contain a parsable XMLBigInteger.
     */

    XMLBigInteger
    (
        const XMLCh* const strValue
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    ~XMLBigInteger();

    XMLBigInteger(const XMLBigInteger& toCopy);

    static XMLCh* getCanonicalRepresentation
                        (
                          const XMLCh*         const rawData
                        ,       MemoryManager* const memMgr = XMLPlatformUtils::fgMemoryManager
                        );

    static void parseBigInteger(const XMLCh* const toConvert
                              , XMLCh* const       retBuffer
                              , int&   signValue
                              , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    static int  compareValues(const XMLBigInteger* const lValue
                             ,const XMLBigInteger* const rValue
                             , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);


    void        multiply(const unsigned int byteToShift);

    void        divide(const unsigned int byteToShift);

    int         getTotalDigit() const;

    /**
     *
     *  Deprecated: please use getRawData
     *
     *  Return a copy of the fMagnitue.
     *  A leading sign is ALWAYS in place and the caller of this method
     *  is responsible for the de-allocation of the memory.
     */
    inline XMLCh*      toString() const;
    
    /**
     *  Return a copy of the fMagnitue.
     *  This is similar to toString, except the internal buffer is returned directly
     *  Caller is not required to delet the returned memory.
     */
    inline XMLCh*      getRawData() const;

    /**
     * Compares this object to the specified object.
     * The result is <code>true</code> if and only if the argument is not
     * <code>null</code> and is an <code>XMLBigInteger</code> object that contains
     * the same <code>int</code> value as this object.
     *
     * @param   toCompare   the object to compare with.
     * @return  <code>true</code> if the objects are the same;
     *          <code>false</code> otherwise.
     */
    bool operator==(const XMLBigInteger& toCompare) const;

    /**
     * Returns the signum function of this number (i.e., -1, 0 or 1 as
     * the value of this number is negative, zero or positive).
     */
    int getSign() const;

    int intValue() const;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------    
    XMLBigInteger& operator=(const XMLBigInteger&);


    void setSign(int);

    /*
     * The number is internally stored in "minimal" sign-fMagnitude format
     * (i.e., no BigIntegers have a leading zero byte in their magnitudes).
     * Zero is represented with a signum of 0 (and a zero-length fMagnitude).
     * Thus, there is exactly one representation for each value.
     */
    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fSign
    //     to represent the sign of the number.
    //
    //  fMagnitude
    //     the buffer holding the number.
    //
    //  fRawData
    //     to preserve the original string used to construct this object,
    //     needed for pattern matching.
    //
    // -----------------------------------------------------------------------

    int         fSign;
    XMLCh*      fMagnitude;  //null terminated
    XMLCh*      fRawData;
    MemoryManager* fMemoryManager;
};

inline int XMLBigInteger::getSign() const
{    
    return fSign;
}

inline int XMLBigInteger::getTotalDigit() const
{
    return ((getSign() ==0) ? 0 : XMLString::stringLen(fMagnitude));
}

inline bool XMLBigInteger::operator==(const XMLBigInteger& toCompare) const
{
    return ( compareValues(this, &toCompare, fMemoryManager) ==0 ? true : false);
}

inline void XMLBigInteger::setSign(int newSign)
{
    fSign = newSign;
}

inline XMLCh*  XMLBigInteger::getRawData() const
{
    return fRawData;
}

//
// The caller needs to de-allocate the memory allocated by this function
//
inline XMLCh*  XMLBigInteger::toString() const
{
    // Return data using global operator new
    return XMLString::replicate(fRawData, fMemoryManager);
}

XERCES_CPP_NAMESPACE_END

#endif
