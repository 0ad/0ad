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
 * $Id: FieldValueMap.hpp,v 1.7 2003/05/26 22:05:01 knoaman Exp $
 */

#if !defined(FIELDVALUEMAP_HPP)
#define FIELDVALUEMAP_HPP

/**
  * This class maps values associated with fields of an identity constraint
  * that have successfully matched some string in an instance document.
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/ValueVectorOf.hpp>
#include <xercesc/util/RefArrayVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class IC_Field;
class DatatypeValidator;


class VALIDATORS_EXPORT FieldValueMap : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    FieldValueMap(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    FieldValueMap(const FieldValueMap& other);
	~FieldValueMap();

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    DatatypeValidator* getDatatypeValidatorAt(const unsigned int index) const;
    DatatypeValidator* getDatatypeValidatorFor(const IC_Field* const key) const;
    XMLCh* getValueAt(const unsigned int index) const;
    XMLCh* getValueFor(const IC_Field* const key) const;
    IC_Field* keyAt(const unsigned int index) const;

	// -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void put(IC_Field* const key, DatatypeValidator* const dv,
             const XMLCh* const value);

	// -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    unsigned int size() const;
    int indexOf(const IC_Field* const key) const;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented operators
    // -----------------------------------------------------------------------
    FieldValueMap& operator= (const FieldValueMap& other);

    // -----------------------------------------------------------------------
    //  Data
    // -----------------------------------------------------------------------
    ValueVectorOf<IC_Field*>*          fFields;
    ValueVectorOf<DatatypeValidator*>* fValidators;
    RefArrayVectorOf<XMLCh>*           fValues;
    MemoryManager*                     fMemoryManager;
};


// ---------------------------------------------------------------------------
//  FieldValueMap: Getter methods
// ---------------------------------------------------------------------------
inline DatatypeValidator*
FieldValueMap::getDatatypeValidatorAt(const unsigned int index) const {

    if (fValidators) {
        return fValidators->elementAt(index);
    }

    return 0;
}

inline DatatypeValidator*
FieldValueMap::getDatatypeValidatorFor(const IC_Field* const key) const {

    if (fValidators) {
        return fValidators->elementAt(indexOf(key));
    }

    return 0;
}

inline XMLCh* FieldValueMap::getValueAt(const unsigned int index) const {

    if (fValues) {
        return fValues->elementAt(index);
    }

    return 0;
}

inline XMLCh* FieldValueMap::getValueFor(const IC_Field* const key) const {

    if (fValues) {
        return fValues->elementAt(indexOf(key));
    }

    return 0;
}

inline IC_Field* FieldValueMap::keyAt(const unsigned int index) const {

    if (fFields) {
        return fFields->elementAt(index);
    }

    return 0;
}

// ---------------------------------------------------------------------------
//  FieldValueMap: Helper methods
// ---------------------------------------------------------------------------
inline unsigned int FieldValueMap::size() const {

    if (fFields) {
        return fFields->size();
    }

    return 0;
}

// ---------------------------------------------------------------------------
//  FieldValueMap: Setter methods
// ---------------------------------------------------------------------------
inline void FieldValueMap::put(IC_Field* const key,
                               DatatypeValidator* const dv,
                               const XMLCh* const value) {

    if (!fFields) {
        fFields = new (fMemoryManager) ValueVectorOf<IC_Field*>(4, fMemoryManager);
        fValidators = new (fMemoryManager) ValueVectorOf<DatatypeValidator*>(4, fMemoryManager);
        fValues = new (fMemoryManager) RefArrayVectorOf<XMLCh>(4, true, fMemoryManager);
    }

    int keyIndex = indexOf(key);

    if (keyIndex == -1) {

        fFields->addElement(key);
        fValidators->addElement(dv);
        fValues->addElement(XMLString::replicate(value, fMemoryManager));
    }
    else {
        fValidators->setElementAt(dv, keyIndex);
        fValues->setElementAt(XMLString::replicate(value, fMemoryManager), keyIndex);
    }
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file FieldValueMap.hpp
  */

