/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2003 The Apache Software Foundation.  All rights
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
 * $Log: XSObjectFactory.hpp,v $
 * Revision 1.8  2004/01/07 02:33:56  knoaman
 * PSVI: inherit facets from base type
 *
 * Revision 1.7  2003/12/29 16:15:41  knoaman
 * More PSVI updates
 *
 * Revision 1.6  2003/12/24 17:42:02  knoaman
 * Misc. PSVI updates
 *
 * Revision 1.5  2003/12/17 20:50:35  knoaman
 * PSVI: fix for annotation of attributes in attributeGroup/derived types
 *
 * Revision 1.4  2003/11/25 18:11:54  knoaman
 * Make XSObjectFactory inherit from XMemory. Thanks to David Cargill.
 *
 * Revision 1.3  2003/11/24 15:45:36  knoaman
 * PSVI: finish construction of XSSimpleTypeDefinition
 *
 * Revision 1.2  2003/11/23 16:21:40  knoaman
 * PSVI: create local elements of complex types
 *
 * Revision 1.1  2003/11/21 17:11:24  knoaman
 * Initial revision
 *
 */

#if !defined(XSOBJECTFACTORY_HPP)
#define XSOBJECTFACTORY_HPP

#include <xercesc/framework/psvi/XSConstants.hpp>
#include <xercesc/util/RefHashTableOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XSObject;
class XSAttributeUse;
class XSAttributeDeclaration;
class XSModel;
class XSElementDeclaration;
class XSSimpleTypeDefinition;
class XSComplexTypeDefinition;
class XSModelGroupDefinition;
class XSAttributeGroupDefinition;
class XSWildcard;
class XSParticle;
class XSAnnotation;
class XSNamespaceItem;
class XSNotationDeclaration;
class SchemaAttDef;
class SchemaElementDecl;
class DatatypeValidator;
class ContentSpecNode;
class ComplexTypeInfo;
class XercesGroupInfo;
class XercesAttGroupInfo;
class XSIDCDefinition;
class IdentityConstraint;
class XMLNotationDecl;

/**
 * Factory class to create various XSObject(s)
 * Used by XSModel
 */
class XMLPARSER_EXPORT XSObjectFactory : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    XSObjectFactory(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~XSObjectFactory();

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and destructor
    // -----------------------------------------------------------------------
    XSObjectFactory(const XSObjectFactory&);
    XSObjectFactory& operator=(const XSObjectFactory&);

    // -----------------------------------------------------------------------
    //  factory methods
    // -----------------------------------------------------------------------
    XSParticle* createModelGroupParticle
    (
        const ContentSpecNode* const node
        , XSModel* const             xsModel
    );

    XSAttributeDeclaration* addOrFind
    (
        SchemaAttDef* const attDef
        , XSModel* const xsModel
        , XSComplexTypeDefinition* const enclosingTypeDef = 0
    );

    XSSimpleTypeDefinition* addOrFind
    (
        DatatypeValidator* const validator
        , XSModel* const xsModel
        , bool isAnySimpleType = false
    );

    XSElementDeclaration* addOrFind
    (
        SchemaElementDecl* const elemDecl
        , XSModel* const xsModel
        , XSComplexTypeDefinition* const enclosingTypeDef = 0
    );

    XSComplexTypeDefinition* addOrFind
    (
        ComplexTypeInfo* const typeInfo
        , XSModel* const xsModel
    );

    XSIDCDefinition* addOrFind
    (
        IdentityConstraint* const ic
        , XSModel* const xsModel
    );

    XSNotationDeclaration* addOrFind
    (
        XMLNotationDecl* const notDecl
        , XSModel* const xsModel
    );

    XSAttributeUse* createXSAttributeUse
    (
        XSAttributeDeclaration* const xsAttDecl
        , XSModel* const xsModel
    );
    XSWildcard* createXSWildcard
    (
        SchemaAttDef* const attDef
        , XSModel* const xsModel
    );

    XSWildcard* createXSWildcard
    (
        const ContentSpecNode* const rootNode
        , XSModel* const xsModel
    );

    XSModelGroupDefinition* createXSModelGroupDefinition
    (
        XercesGroupInfo* const groupInfo
        , XSModel* const xsModel
    );

    XSAttributeGroupDefinition* createXSAttGroupDefinition
    (
        XercesAttGroupInfo* const attGroupInfo
        , XSModel* const xsModel
    );

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    // creates a particle corresponding to an element
    XSParticle* createElementParticle
    (
        const ContentSpecNode* const rootNode
        , XSModel* const             xsModel
    );

    // creates a particle corresponding to a wildcard
    XSParticle* createWildcardParticle
    (
        const ContentSpecNode* const rootNode
        , XSModel* const             xsModel
    );

    XSAnnotation* getAnnotationFromModel
    (
        XSModel* const xsModel
        , const void* const key
    );

    void buildAllParticles
    (
        const ContentSpecNode* const rootNode
        , XSParticleList* const particleList
        , XSModel* const xsModel
    );

    void buildChoiceSequenceParticles
    (
        const ContentSpecNode* const rootNode
        , XSParticleList* const particleList
        , XSModel* const xsModel
    );

    void putObjectInMap
    (
        void* key
        , XSObject* const object
    );

    XSObject* getObjectFromMap
    (
        void* key
    );

    void processFacets
    (
        DatatypeValidator* const dv
        , XSModel* const xsModel
        , XSSimpleTypeDefinition* const xsST
    );

    void processAttUse
    (
        SchemaAttDef* const attDef
        , XSAttributeUse* const xsAttUse
    );

    bool isMultiValueFacetDefined(DatatypeValidator* const dv);

    // make XSModel our friend
    friend class XSModel;

    // -----------------------------------------------------------------------
    //  Private Data Members
    //
    //  fMemoryManager
    //      The memory manager used to create various XSObject(s).
    // -----------------------------------------------------------------------
    MemoryManager*            fMemoryManager;
    RefHashTableOf<XSObject>* fXercesToXSMap;
    RefVectorOf<XSObject>*    fDeleteVector;
};

inline XSObject* XSObjectFactory::getObjectFromMap(void* key)
{
    return fXercesToXSMap->get(key);
}


XERCES_CPP_NAMESPACE_END

#endif
