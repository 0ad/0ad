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
 * $Id: XercesGroupInfo.hpp,v 1.8 2003/11/20 17:06:05 knoaman Exp $
 */

#if !defined(XERCESGROUPINFO_HPP)
#define XERCESGROUPINFO_HPP


/**
  * The class act as a place holder to store group information.
  *
  * The class is intended for internal use.
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/validators/schema/SchemaElementDecl.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declarations
// ---------------------------------------------------------------------------
class ContentSpecNode;
class XSDLocator;


class VALIDATORS_EXPORT XercesGroupInfo : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Public Constructors/Destructor
    // -----------------------------------------------------------------------
    XercesGroupInfo
    (
        unsigned int groupNameId
        , unsigned int groupNamespaceId
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    ~XercesGroupInfo();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    bool                     getCheckElementConsistency() const;
    int                      getScope() const;
    unsigned int             elementCount() const;
    ContentSpecNode*         getContentSpec() const;
    SchemaElementDecl*       elementAt(const unsigned int index);
    const SchemaElementDecl* elementAt(const unsigned int index) const;
    XSDLocator*              getLocator() const;
    XercesGroupInfo*         getBaseGroup() const;
    unsigned int             getNameId() const;
    unsigned int             getNamespaceId() const;

	// -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setScope(const int other);
    void setContentSpec(ContentSpecNode* const other);
    void addElement(SchemaElementDecl* const toAdd);
    void setLocator(XSDLocator* const aLocator);
    void setBaseGroup(XercesGroupInfo* const baseGroup);
    void setCheckElementConsistency(const bool aValue);

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XercesGroupInfo)
    XercesGroupInfo(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    XercesGroupInfo(const XercesGroupInfo& elemInfo);
    XercesGroupInfo& operator= (const XercesGroupInfo& other);

    // -----------------------------------------------------------------------
    //  Private data members
    // -----------------------------------------------------------------------
    bool                            fCheckElementConsistency;
    int                             fScope;
    unsigned int                    fNameId;
    unsigned int                    fNamespaceId;
    ContentSpecNode*                fContentSpec;
    RefVectorOf<SchemaElementDecl>* fElements;
    XercesGroupInfo*                fBaseGroup; // redefine by restriction
    XSDLocator*                     fLocator;
};

// ---------------------------------------------------------------------------
//  XercesGroupInfo: Getter methods
// ---------------------------------------------------------------------------
inline int XercesGroupInfo::getScope() const {

    return fScope;
}

inline unsigned int XercesGroupInfo::elementCount() const {

    return fElements->size();
}

inline ContentSpecNode* XercesGroupInfo::getContentSpec() const {

    return fContentSpec;
}

inline SchemaElementDecl*
XercesGroupInfo::elementAt(const unsigned int index) {

    return fElements->elementAt(index);
}

inline const SchemaElementDecl*
XercesGroupInfo::elementAt(const unsigned int index) const {

    return fElements->elementAt(index);
}

inline XSDLocator* XercesGroupInfo::getLocator() const {

    return fLocator;
}

inline XercesGroupInfo* XercesGroupInfo::getBaseGroup() const {

    return fBaseGroup;
}

inline bool XercesGroupInfo::getCheckElementConsistency() const {

    return fCheckElementConsistency;
}

inline unsigned int XercesGroupInfo::getNameId() const
{
    return fNameId;
}

inline unsigned int XercesGroupInfo::getNamespaceId() const
{
    return fNamespaceId;
}

// ---------------------------------------------------------------------------
//  XercesGroupInfo: Setter methods
// ---------------------------------------------------------------------------}
inline void XercesGroupInfo::setScope(const int other) {

    fScope = other;
}

inline void XercesGroupInfo::setContentSpec(ContentSpecNode* const other) {

    fContentSpec = other;
}

inline void XercesGroupInfo::addElement(SchemaElementDecl* const elem) {

    if (!fElements->containsElement(elem))
        fElements->addElement(elem);
}

inline void XercesGroupInfo::setBaseGroup(XercesGroupInfo* const baseGroup) {

    fBaseGroup = baseGroup;
}

inline void XercesGroupInfo::setCheckElementConsistency(const bool aValue) {

    fCheckElementConsistency = aValue;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file XercesGroupInfo.hpp
  */

