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
 * $Id: XercesAttGroupInfo.hpp,v 1.8 2003/12/17 20:50:35 knoaman Exp $
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

