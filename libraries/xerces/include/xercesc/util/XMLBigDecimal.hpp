/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2003 The Apache Software Foundation.  All rights
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
 * $Id: XMLBigDecimal.hpp,v 1.17 2004/01/13 19:50:56 peiyongz Exp $
 */

#ifndef XML_BIGDECIMAL_HPP
#define XML_BIGDECIMAL_HPP

#include <xercesc/util/XMLNumber.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT XMLBigDecimal : public XMLNumber
{
public:

    /**
     * Constructs a newly allocated <code>XMLBigDecimal</code> object that
     * represents the value represented by the string.
     *
     * @param  strValue the <code>String</code> to be converted to an
     *                  <code>XMLBigDecimal</code>.
     * @param  manager  Pointer to the memory manager to be used to
     *                  allocate objects.
     * @exception  NumberFormatException  if the <code>String</code> does not
     *               contain a parsable XMLBigDecimal.
     */

    XMLBigDecimal
    (
        const XMLCh* const strValue
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    ~XMLBigDecimal();

    static int            compareValues(const XMLBigDecimal* const lValue
                                      , const XMLBigDecimal* const rValue
                                      , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    static XMLCh* getCanonicalRepresentation
                  (
                   const XMLCh*         const rawData
                 ,       MemoryManager* const memMgr = XMLPlatformUtils::fgMemoryManager
                  );

    static void  parseDecimal
                ( 
                   const XMLCh* const toParse
                ,        XMLCh* const retBuffer
                ,        int&         sign
                ,        int&         totalDigits
                ,        int&         fractDigits
                ,        MemoryManager* const manager
                );

    /**
     *
     *  Deprecated: please use getRawData
     *
     */
    virtual XMLCh*        toString() const;
    
    virtual XMLCh*        getRawData() const;

    virtual const XMLCh*  getFormattedString() const;

    virtual int           getSign() const;

    const XMLCh*          getValue() const;

    unsigned int          getScale() const;

    unsigned int          getTotalDigit() const;

    /**
     * Compares this object to the specified object.
     *
     * @param   other   the object to compare with.
     * @return  <code>-1</code> value is less than other's
     *          <code>0</code>  value equals to other's
     *          <code>+1</code> value is greater than other's
     */
     int toCompare(const XMLBigDecimal& other) const;

    /*
     * Sets the value to be converted
     *
     * @param   strValue the value to convert
     */
    void setDecimalValue(const XMLCh* const strValue);

    MemoryManager* getMemoryManager() const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XMLBigDecimal)

    XMLBigDecimal(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:

    void  cleanUp();
    
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------       
    XMLBigDecimal(const XMLBigDecimal& other);
    XMLBigDecimal& operator=(const XMLBigDecimal& other);        
    
    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fSign
    //     sign
    //
    //  fTotalDigits
    //     the total number of didits 
    //
    //  fScale
    //     the number of digits to the right of the decimal point
    //
    //  fIntVal
    //     The value of this BigDecimal, w/o
    //         leading whitespace, leading zero
    //         decimal point
    //         trailing zero, trailing whitespace
    //
    //  fRawData
    //     to preserve the original string used to construct this object,
    //     needed for pattern matching.
    //
    // -----------------------------------------------------------------------
    int            fSign;
    unsigned int   fTotalDigits;
    unsigned int   fScale;
    unsigned int   fRawDataLen;
    XMLCh*         fRawData;
    XMLCh*         fIntVal;
    MemoryManager* fMemoryManager;
};

inline int XMLBigDecimal::getSign() const
{
    return fSign;
}

inline const XMLCh* XMLBigDecimal::getValue() const
{
    return fIntVal;
}

inline unsigned int XMLBigDecimal::getScale() const
{
    return fScale;
}

inline unsigned int XMLBigDecimal::getTotalDigit() const
{
    return fTotalDigits;
}

inline XMLCh*  XMLBigDecimal::getRawData() const
{
    return fRawData;
}

inline const XMLCh*  XMLBigDecimal::getFormattedString() const
{
    return fRawData;
}

inline MemoryManager* XMLBigDecimal::getMemoryManager() const
{
    return fMemoryManager;
}

//
// The caller needs to de-allocate the memory allocated by this function
//
inline XMLCh*  XMLBigDecimal::toString() const
{
    // Return data using global operator new
    return XMLString::replicate(fRawData);
}

XERCES_CPP_NAMESPACE_END

#endif
