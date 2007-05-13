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
 * $Id: IdentityConstraint.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(IDENTITYCONSTRAINT_HPP)
#define IDENTITYCONSTRAINT_HPP


/**
  * The class act as a base class for schema identity constraints.
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/validators/schema/identity/IC_Field.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declarations
// ---------------------------------------------------------------------------
class IC_Selector;

class VALIDATORS_EXPORT IdentityConstraint : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constants
    // -----------------------------------------------------------------------
    enum ICType {
        UNIQUE = 0,
        KEY = 1,
        KEYREF = 2,
        UNKNOWN
    };

    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
	virtual ~IdentityConstraint();

    // -----------------------------------------------------------------------
    //  operators
    // -----------------------------------------------------------------------
    bool operator== (const IdentityConstraint& other) const;
    bool operator!= (const IdentityConstraint& other) const;

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    virtual short getType() const = 0;
    int           getFieldCount() const;
    XMLCh*        getIdentityConstraintName() const;
    XMLCh*        getElementName() const;
    IC_Selector*  getSelector() const;
    int           getNamespaceURI() const;

	// -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setSelector(IC_Selector* const selector);
    void setNamespaceURI(int uri);

	// -----------------------------------------------------------------------
    //  Access methods
    // -----------------------------------------------------------------------
    void addField(IC_Field* const field);
    const IC_Field* getFieldAt(const unsigned int index) const;
    IC_Field* getFieldAt(const unsigned int index);

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(IdentityConstraint)

	static void                storeIC(XSerializeEngine&         serEng
                                     , IdentityConstraint* const ic);

	static IdentityConstraint* loadIC(XSerializeEngine& serEng);

protected:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    IdentityConstraint(const XMLCh* const identityConstraintName,
                       const XMLCh* const elementName,
					   MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    IdentityConstraint(const IdentityConstraint& other);
    IdentityConstraint& operator= (const IdentityConstraint& other);

    // -----------------------------------------------------------------------
    //  CleanUp methods
    // -----------------------------------------------------------------------
    void cleanUp();

    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fIdentityConstraintName
    //      The identity constraint name
    //
    //  fElemName
    //      The element name
    //
    //  fSelector
    //      The selector information
    //
    //  fFields
    //      The field(s) information
    // -----------------------------------------------------------------------
    XMLCh*                 fIdentityConstraintName;
    XMLCh*                 fElemName;
    IC_Selector*           fSelector;
    RefVectorOf<IC_Field>* fFields;
    MemoryManager*         fMemoryManager;
    int                    fNamespaceURI;
};


// ---------------------------------------------------------------------------
//  IdentityConstraint: Getter methods
// ---------------------------------------------------------------------------
inline int IdentityConstraint::getFieldCount() const {

    if (fFields) {
        return fFields->size();
    }

    return 0;
}

inline XMLCh* IdentityConstraint::getIdentityConstraintName() const {

    return fIdentityConstraintName;
}

inline XMLCh* IdentityConstraint::getElementName() const {

    return fElemName;
}

inline IC_Selector* IdentityConstraint::getSelector() const {

    return fSelector;
}

inline int IdentityConstraint::getNamespaceURI() const
{
    return fNamespaceURI;
}

// ---------------------------------------------------------------------------
//  IdentityConstraint: Setter methods
// ---------------------------------------------------------------------------
inline void IdentityConstraint::setNamespaceURI(int uri)
{
    fNamespaceURI = uri;
}

// ---------------------------------------------------------------------------
//  IdentityConstraint: Access methods
// ---------------------------------------------------------------------------
inline void IdentityConstraint::addField(IC_Field* const field) {

    if (!fFields) {
        fFields = new (fMemoryManager) RefVectorOf<IC_Field>(4, true, fMemoryManager);
    }

    fFields->addElement(field);
}

inline const IC_Field* IdentityConstraint::getFieldAt(const unsigned int index) const {

    if (fFields) {
        return (fFields->elementAt(index));
    }

    return 0;
}

inline IC_Field* IdentityConstraint::getFieldAt(const unsigned int index) {

    if (fFields) {
        return (fFields->elementAt(index));
    }

    return 0;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file IdentityConstraint.hpp
  */

