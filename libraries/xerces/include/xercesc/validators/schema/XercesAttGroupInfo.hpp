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
 * $Id: XercesAttGroupInfo.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(XERCESATTGROUPINFO_HPP)
#define XERCESATTGROUPINFO_HPP


/**
  * The class act as a place holder to store attributeGroup information.
  *
  * The class is intended for internal use.
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/validators/schema/SchemaAttDef.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT XercesAttGroupInfo : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Public Constructors/Destructor
    // -----------------------------------------------------------------------
    XercesAttGroupInfo
    (
        unsigned int           attGroupNameId
        , unsigned int         attGroupNamespaceId
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    ~XercesAttGroupInfo();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    bool                containsTypeWithId() const;
    unsigned int        attributeCount() const;
    unsigned int        anyAttributeCount() const;
    unsigned int        getNameId() const;
    unsigned int        getNamespaceId() const;
    SchemaAttDef*       attributeAt(const unsigned int index);
    const SchemaAttDef* attributeAt(const unsigned int index) const;
    SchemaAttDef*       anyAttributeAt(const unsigned int index);
    const SchemaAttDef* anyAttributeAt(const unsigned int index) const;
    SchemaAttDef*       getCompleteWildCard() const;
    const SchemaAttDef* getAttDef(const XMLCh* const baseName,
                                  const int uriId) const;

	// -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setTypeWithId(const bool other);
    void addAttDef(SchemaAttDef* const toAdd, const bool toClone = false);
    void addAnyAttDef(SchemaAttDef* const toAdd, const bool toClone = false);
    void setCompleteWildCard(SchemaAttDef* const toSet);

	// -----------------------------------------------------------------------
    //  Query methods
    // -----------------------------------------------------------------------
    bool containsAttribute(const XMLCh* const name, const unsigned int uri);

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XercesAttGroupInfo)
    XercesAttGroupInfo(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    XercesAttGroupInfo(const XercesAttGroupInfo& elemInfo);
    XercesAttGroupInfo& operator= (const XercesAttGroupInfo& other);

    // -----------------------------------------------------------------------
    //  Private data members
    // -----------------------------------------------------------------------
    bool                       fTypeWithId;
    unsigned int               fNameId;
    unsigned int               fNamespaceId;
    RefVectorOf<SchemaAttDef>* fAttributes;
    RefVectorOf<SchemaAttDef>* fAnyAttributes;
    SchemaAttDef*              fCompleteWildCard;
    MemoryManager*             fMemoryManager;
};

// ---------------------------------------------------------------------------
//  XercesAttGroupInfo: Getter methods
// ---------------------------------------------------------------------------
inline bool XercesAttGroupInfo::containsTypeWithId() const {

    return fTypeWithId;
}

inline unsigned int XercesAttGroupInfo::attributeCount() const {

    if (fAttributes) {
        return fAttributes->size();
    }

    return 0;
}

inline unsigned int XercesAttGroupInfo::anyAttributeCount() const {

    if (fAnyAttributes) {
        return fAnyAttributes->size();
    }

    return 0;
}

inline unsigned int XercesAttGroupInfo::getNameId() const
{
    return fNameId;
}

inline unsigned int XercesAttGroupInfo::getNamespaceId() const
{
    return fNamespaceId;
}

inline SchemaAttDef*
XercesAttGroupInfo::attributeAt(const unsigned int index) {

    if (fAttributes) {
        return fAttributes->elementAt(index);
    }

    return 0;
}

inline const SchemaAttDef*
XercesAttGroupInfo::attributeAt(const unsigned int index) const {

    if (fAttributes) {
        return fAttributes->elementAt(index);
    }

    return 0;
}

inline SchemaAttDef*
XercesAttGroupInfo::anyAttributeAt(const unsigned int index) {

    if (fAnyAttributes) {
        return fAnyAttributes->elementAt(index);
    }

    return 0;
}

inline const SchemaAttDef*
XercesAttGroupInfo::anyAttributeAt(const unsigned int index) const {

    if (fAnyAttributes) {
        return fAnyAttributes->elementAt(index);
    }

    return 0;
}

inline SchemaAttDef*
XercesAttGroupInfo::getCompleteWildCard() const {

    return fCompleteWildCard;
}

// ---------------------------------------------------------------------------
//  XercesAttGroupInfo: Setter methods
// ---------------------------------------------------------------------------
inline void XercesAttGroupInfo::setTypeWithId(const bool other) {

    fTypeWithId = other;
}

inline void XercesAttGroupInfo::addAttDef(SchemaAttDef* const toAdd,
                                             const bool toClone) {

    if (!fAttributes) {
        fAttributes = new (fMemoryManager) RefVectorOf<SchemaAttDef>(4, true, fMemoryManager);
    }

    if (toClone) {
        SchemaAttDef* clonedAttDef = new (fMemoryManager) SchemaAttDef(toAdd);

        if (!clonedAttDef->getBaseAttDecl())
            clonedAttDef->setBaseAttDecl(toAdd);

        fAttributes->addElement(clonedAttDef);
    }
    else {
        fAttributes->addElement(toAdd);
    }
}

inline void XercesAttGroupInfo::addAnyAttDef(SchemaAttDef* const toAdd,
                                             const bool toClone) {

    if (!fAnyAttributes) {
        fAnyAttributes = new (fMemoryManager) RefVectorOf<SchemaAttDef>(2, true, fMemoryManager);
    }

    if (toClone) {
        SchemaAttDef* clonedAttDef = new (fMemoryManager) SchemaAttDef(toAdd);

        if (!clonedAttDef->getBaseAttDecl())
            clonedAttDef->setBaseAttDecl(toAdd);

        fAnyAttributes->addElement(clonedAttDef);
    }
    else {
        fAnyAttributes->addElement(toAdd);
    }
}

inline void
XercesAttGroupInfo::setCompleteWildCard(SchemaAttDef* const toSet) {

    if (fCompleteWildCard) {
        delete fCompleteWildCard;
    }

    fCompleteWildCard = toSet;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file XercesAttGroupInfo.hpp
  */

