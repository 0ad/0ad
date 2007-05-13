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
 * $Id: ValueStore.hpp 176287 2005-01-12 20:06:55Z cargilld $
 */

#if !defined(VALUESTORE_HPP)
#define VALUESTORE_HPP

/**
  * This class stores values associated to an identity constraint.
  * Each value stored corresponds to a field declared for the identity
  * constraint.
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/schema/identity/FieldValueMap.hpp>
#include <xercesc/util/RefVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class FieldActivator;
class IdentityConstraint;
class XMLScanner;
class ValueStoreCache;


class VALIDATORS_EXPORT ValueStore : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    ValueStore(IdentityConstraint* const ic,
               XMLScanner* const scanner,
               MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~ValueStore();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    IdentityConstraint* getIdentityConstraint() const;

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    void append(const ValueStore* const other);
    void startValueScope();
    void endValueScope();
    void addValue(FieldActivator* const fieldActivator,
                  IC_Field* const field,
                  DatatypeValidator* const dv,
                  const XMLCh* const value);
    bool contains(const FieldValueMap* const other);

    /**
      * @deprecated
      */
    void addValue(IC_Field* const field, DatatypeValidator* const dv,
                  const XMLCh* const value);


    // -----------------------------------------------------------------------
    //  Document handling methods
    // -----------------------------------------------------------------------
    void endDcocumentFragment(ValueStoreCache* const valueStoreCache);

    // -----------------------------------------------------------------------
    //  Error reporting methods
    // -----------------------------------------------------------------------
    void duplicateValue();
    void reportNilError(IdentityConstraint* const ic);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    ValueStore(const ValueStore& other);
    ValueStore& operator= (const ValueStore& other);

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    /**
      * Returns whether a field associated <DatatypeValidator, String> value
      * is a duplicate of another associated value.
      * It is a duplicate only if either of these conditions are true:
      * - The Datatypes are the same or related by derivation and the values
      *   are in the same valuespace.
      * - The datatypes are unrelated and the values are Stringwise identical.
      */
    bool isDuplicateOf(DatatypeValidator* const dv1, const XMLCh* const val1,
                       DatatypeValidator* const dv2, const XMLCh* const val2);


    // -----------------------------------------------------------------------
    //  Data
    // -----------------------------------------------------------------------
    bool                        fDoReportError;
    int                         fValuesCount;
    IdentityConstraint*         fIdentityConstraint;
    FieldValueMap               fValues;
    RefVectorOf<FieldValueMap>* fValueTuples;
    ValueStore*                 fKeyValueStore;
    XMLScanner*                 fScanner; // for error reporting - REVISIT
    MemoryManager*              fMemoryManager;
};

// ---------------------------------------------------------------------------
//  ValueStore: Getter methods
// ---------------------------------------------------------------------------
inline IdentityConstraint*
ValueStore::getIdentityConstraint() const {
    return fIdentityConstraint;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file ValueStore.hpp
  */

