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
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: XMLNumber.hpp,v 1.13 2004/01/29 11:48:47 cargilld Exp $
 * $Log: XMLNumber.hpp,v $
 * Revision 1.13  2004/01/29 11:48:47  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.12  2004/01/13 19:50:56  peiyongz
 * remove parseContent()
 *
 * Revision 1.10  2003/10/17 21:10:55  peiyongz
 * loadNumber() added
 *
 * Revision 1.9  2003/10/02 19:18:12  peiyongz
 * enum NumberType
 *
 * Revision 1.8  2003/09/23 18:16:07  peiyongz
 * Inplementation for Serialization/Deserialization
 *
 * Revision 1.7  2003/05/15 19:07:46  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.6  2003/05/09 15:13:46  peiyongz
 * Deprecated toString() in XMLNumber family
 *
 * Revision 1.5  2003/03/10 20:55:58  peiyongz
 * Schema Errata E2-40 double/float
 *
 * Revision 1.4  2003/02/02 23:54:43  peiyongz
 * getFormattedString() added to return original and converted value.
 *
 * Revision 1.3  2003/01/30 21:55:22  tng
 * Performance: create getRawData which is similar to toString but return the internal data directly, user is not required to delete the returned memory.
 *
 * Revision 1.2  2002/11/04 15:22:05  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:15  peiyongz
 * sane_include
 *
 * Revision 1.2  2001/10/09 21:28:28  peiyongz
 * explicit ctor/dtor defined.
 *
 * Revision 1.1  2001/09/27 14:54:03  peiyongz
 * DTV Reorganization: new class
 *
 */

#ifndef XMLNUMBER_HPP
#define XMLNUMBER_HPP

#include <xercesc/internal/XSerializable.hpp>
#include <xercesc/util/XMemory.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT XMLNumber : public XSerializable, public XMemory
{
public:

    enum
    {
        LESS_THAN     = -1,
        EQUAL         = 0,
        GREATER_THAN  = 1,
        INDETERMINATE = 2
    };

    enum NumberType {
        Float,
        Double,
        BigDecimal,
        DateTime,
        UnKnown
    };

    virtual ~XMLNumber();

    /**
     *  Deprecated: please use getRawData
     *
     *  Return string representation of the decimal value.
     *  A decimal point will be included as necessary,
     *  the caller of this method is responsible for the
     *  de-allocation of the memory.
     */
    virtual XMLCh*     toString() const = 0;
    
	/**
	 *  Return string representation of the decimal value.
     *  A decimal point will be included as necessary.
     *  Similar to toString above, but the internal buffer is
     *  returned directly, user is not required to delete
     *  the returned buffer
	 */
    virtual XMLCh*     getRawData() const = 0;    

    /**
     *  Return the original and converted value of the original data.
     *  (applicable to double/float)
     *
     *  The internal buffer is returned directly, user is not required 
     *  to delete the returned buffer
     */
    virtual const XMLCh*     getFormattedString() const = 0;    

	/**
	 * Returns the sign of this number
     *
     * -1   negative
     *  0   zero
     *  1   positive
	 *
	 */
    virtual int        getSign() const = 0;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XMLNumber)

    static XMLNumber*          loadNumber(XMLNumber::NumberType   numType
                                        , XSerializeEngine&       serEng);

protected:

    XMLNumber();

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLNumber(const XMLNumber&);
    XMLNumber& operator=(const XMLNumber&);
};

XERCES_CPP_NAMESPACE_END

#endif
