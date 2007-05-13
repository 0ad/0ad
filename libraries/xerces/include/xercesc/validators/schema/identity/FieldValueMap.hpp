/*
 * Copyright 2001-2004 The Apache Software Foundation.
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
 * $Id: FieldValueMap.hpp 191712 2005-06-21 19:08:15Z cargilld $
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
    //  Private helper methods
    // -----------------------------------------------------------------------
    void cleanUp();

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

