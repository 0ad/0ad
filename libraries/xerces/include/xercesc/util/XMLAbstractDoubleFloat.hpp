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
 * $Id: XMLAbstractDoubleFloat.hpp,v 1.20 2004/01/29 11:48:46 cargilld Exp $
 * $Log: XMLAbstractDoubleFloat.hpp,v $
 * Revision 1.20  2004/01/29 11:48:46  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.19  2004/01/13 19:50:56  peiyongz
 * remove parseContent()
 *
 * Revision 1.17  2003/12/17 00:18:35  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.16  2003/12/11 21:38:12  peiyongz
 * support for Canonical Representation for Datatype
 *
 * Revision 1.15  2003/10/15 14:50:01  peiyongz
 * Bugzilla#22821: locale-sensitive function used to validate 'double' type, patch
 * from jsweeney@spss.com (Jeff Sweeney)
 *
 * Revision 1.14  2003/09/23 18:16:07  peiyongz
 * Inplementation for Serialization/Deserialization
 *
 * Revision 1.13  2003/05/18 14:02:05  knoaman
 * Memory manager implementation: pass per instance manager.
 *
 * Revision 1.12  2003/05/16 06:01:53  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.11  2003/05/15 19:07:46  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.10  2003/05/09 15:13:46  peiyongz
 * Deprecated toString() in XMLNumber family
 *
 * Revision 1.9  2003/03/10 20:55:58  peiyongz
 * Schema Errata E2-40 double/float
 *
 * Revision 1.8  2003/02/02 23:54:43  peiyongz
 * getFormattedString() added to return original and converted value.
 *
 * Revision 1.7  2003/01/30 21:55:22  tng
 * Performance: create getRawData which is similar to toString but return the internal data directly, user is not required to delete the returned memory.
 *
 * Revision 1.6  2002/12/11 00:20:02  peiyongz
 * Doing businesss in value space. Converting out-of-bound value into special values.
 *
 * Revision 1.5  2002/11/04 15:22:05  tng
 * C++ Namespace Support.
 *
 * Revision 1.4  2002/03/06 19:13:12  peiyongz
 * Patch: more valid lexcial representation for positive/negative zero
 *
 * Revision 1.3  2002/03/01 18:47:37  peiyongz
 * fix: more valid lexcial representation forms for "neural zero"
 *
 * Revision 1.2  2002/02/20 18:17:02  tng
 * [Bug 5977] Warnings on generating apiDocs.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:14  peiyongz
 * sane_include
 *
 * Revision 1.4  2001/11/28 15:39:26  peiyongz
 * return Type& for operator=
 *
 * Revision 1.3  2001/11/22 21:39:00  peiyongz
 * Allow "0.0" to be a valid lexcial representation of ZERO.
 *
 * Revision 1.2  2001/11/22 20:23:00  peiyongz
 * _declspec(dllimport) and inline warning C4273
 *
 * Revision 1.1  2001/11/19 21:33:42  peiyongz
 * Reorganization: Double/Float
 *
 *
 */

#ifndef XML_ABSTRACT_DOUBLE_FLOAT_HPP
#define XML_ABSTRACT_DOUBLE_FLOAT_HPP

#include <xercesc/util/XMLNumber.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/***
 * 3.2.5.1 Lexical representation
 *
 *   double values have a lexical representation consisting of a mantissa followed,
 *   optionally, by the character "E" or "e", followed by an exponent.
 *
 *   The exponent ·must· be an integer.
 *   The mantissa must be a decimal number.
 *   The representations for exponent and mantissa must follow the lexical rules
 *   for integer and decimal.
 *
 *   If the "E" or "e" and the following exponent are omitted,
 *   an exponent value of 0 is assumed.
***/

/***
 * 3.2.4.1 Lexical representation
 *
 *   float values have a lexical representation consisting of a mantissa followed,
 *   optionally, by the character "E" or "e", followed by an exponent.
 *
 *   The exponent ·must· be an integer.
 *   The mantissa must be a decimal number.
 *   The representations for exponent and mantissa must follow the lexical rules
 *   for integer and decimal.
 *
 *   If the "E" or "e" and the following exponent are omitted,
 *   an exponent value of 0 is assumed.
***/

class XMLUTIL_EXPORT XMLAbstractDoubleFloat : public XMLNumber
{
public:

    enum LiteralType
    {
        NegINF,
        PosINF,
        NaN,
        SpecialTypeNum,
        Normal
    };

    virtual ~XMLAbstractDoubleFloat();

    static XMLCh* getCanonicalRepresentation
                        (
                          const XMLCh*         const rawData
                        ,       MemoryManager* const memMgr = XMLPlatformUtils::fgMemoryManager
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

    MemoryManager*        getMemoryManager() const;

    /***
     *
     * The decimal point delimiter for the schema double/float type is
     * defined to be a period and is not locale-specific. So, it must
     * be replaced with the local-specific delimiter before converting
     * from string to double/float.
     *
     ***/
    void                  normalizeDecimalPoint(char* const toNormal);

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XMLAbstractDoubleFloat)

protected:

    //
    // To be used by derived class exclusively
    //
    XMLAbstractDoubleFloat(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    void                  init(const XMLCh* const strValue);

    /**
	 * Compares this object to the specified object.
	 * The result is <code>true</code> if and only if the argument is not
	 * <code>null</code> and is an <code>XMLAbstractDoubleFloat</code> object that contains
	 * the same <code>int</code> value as this object.
	 *
	 * @param   lValue the object to compare with.
	 * @param   rValue the object to compare against.
	 * @return  <code>true</code> if the objects are the same;
	 *          <code>false</code> otherwise.
	 */

    static int            compareValues(const XMLAbstractDoubleFloat* const lValue
                                      , const XMLAbstractDoubleFloat* const rValue
                                      , MemoryManager* const manager);

    //
    // to be overwritten by derived class
    //
    virtual void          checkBoundary(const XMLCh* const strValue) = 0;

private:
    //
    // Unimplemented
    //
    // copy ctor
    // assignment ctor
    //
    XMLAbstractDoubleFloat(const XMLAbstractDoubleFloat& toCopy);
    XMLAbstractDoubleFloat& operator=(const XMLAbstractDoubleFloat& toAssign);

	void                  normalizeZero(XMLCh* const);

    inline bool           isSpecialValue() const;

    static int            compareSpecial(const XMLAbstractDoubleFloat* const specialValue                                       
                                       , MemoryManager* const manager);

    void                  formatString();

protected:
    double                  fValue;
    LiteralType             fType;
    bool                    fDataConverted;

private:
    int                     fSign;
    XMLCh*                  fRawData;

    //
    // If the original string is not lexcially the same as the five
    // special value notations, and the value is converted to
    // special value due underlying platform restriction on data
    // representation, then this string is constructed and
    // takes the form "original_string (special_value_notation)", 
    // otherwise it is empty.
    //
    XMLCh*                  fFormattedString;
    MemoryManager*          fMemoryManager;
};

inline bool XMLAbstractDoubleFloat::isSpecialValue() const
{
    return (fType < SpecialTypeNum);
}

inline MemoryManager* XMLAbstractDoubleFloat::getMemoryManager() const
{
    return fMemoryManager;
}

XERCES_CPP_NAMESPACE_END

#endif
