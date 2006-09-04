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
 * $Id: IC_Field.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(IC_FIELD_HPP)
#define IC_FIELD_HPP


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/schema/identity/XPathMatcher.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class ValueStore;
class FieldActivator;


class VALIDATORS_EXPORT IC_Field : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    IC_Field(XercesXPath* const xpath,
             IdentityConstraint* const identityConstraint);
	~IC_Field();

    // -----------------------------------------------------------------------
    //  operators
    // -----------------------------------------------------------------------
    bool operator== (const IC_Field& other) const;
    bool operator!= (const IC_Field& other) const;

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    XercesXPath* getXPath() const { return fXPath; }
    IdentityConstraint* getIdentityConstraint() const { return fIdentityConstraint; }

    /**
      * @deprecated
      */
    bool getMayMatch() const { return false; }

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    /**
      * @deprecated
      */
    void setMayMatch(const bool) {}

    // -----------------------------------------------------------------------
    //  Factory methods
    // -----------------------------------------------------------------------
    XPathMatcher* createMatcher
    (
        FieldActivator* const fieldActivator
        , ValueStore* const valueStore
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    /**
      * @deprecated
      */
    XPathMatcher* createMatcher(ValueStore* const valueStore,
                                MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(IC_Field)

    IC_Field(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    IC_Field(const IC_Field& other);
    IC_Field& operator= (const IC_Field& other);

    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    XercesXPath*        fXPath;
    IdentityConstraint* fIdentityConstraint;
};


class VALIDATORS_EXPORT FieldMatcher : public XPathMatcher
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    ~FieldMatcher() {}

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    ValueStore* getValueStore() const { return fValueStore; }
    IC_Field*   getField() const { return fField; }

    // -----------------------------------------------------------------------
    //  Virtual methods
    // -----------------------------------------------------------------------
    void matched(const XMLCh* const content, DatatypeValidator* const dv,
                 const bool isNil);

private:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    FieldMatcher(XercesXPath* const anXPath,
                 IC_Field* const aField,
                 ValueStore* const valueStore,
                 FieldActivator* const fieldActivator,
                 MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    FieldMatcher(const FieldMatcher& other);
    FieldMatcher& operator= (const FieldMatcher& other);

    // -----------------------------------------------------------------------
    //  Friends
    // -----------------------------------------------------------------------
    friend class IC_Field;

    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    ValueStore*     fValueStore;
    IC_Field*       fField;
    FieldActivator* fFieldActivator;
};

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file IC_Field.hpp
  */

