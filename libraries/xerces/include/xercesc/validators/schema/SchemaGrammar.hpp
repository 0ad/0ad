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
 * $Log: SchemaGrammar.hpp,v $
 * Revision 1.15  2004/01/29 11:52:31  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.14  2003/12/17 00:18:40  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.13  2003/11/14 22:35:09  neilg
 * changes in support of second phase of XSModel implementation; thanks to David Cargill
 *
 * Revision 1.12  2003/11/12 20:35:31  peiyongz
 * Stateless Grammar: ValidationContext
 *
 * Revision 1.11  2003/11/06 19:28:11  knoaman
 * PSVI support for annotations.
 *
 * Revision 1.10  2003/10/14 15:22:28  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.9  2003/09/22 19:49:03  neilg
 * implement change to Grammar::putElem(XMLElementDecl, bool).  If Grammars are used only to hold declared objects, there will be no need for the fElemNonDeclPool tables; make Grammar implementations lazily create them only if the application requires them (which good cpplications should not.)
 *
 * Revision 1.8  2003/07/31 17:12:10  peiyongz
 * Grammar embed grammar description
 *
 * Revision 1.7  2003/05/16 21:43:21  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.6  2003/05/15 18:57:27  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.5  2002/11/04 14:49:41  tng
 * C++ Namespace Support.
 *
 * Revision 1.4  2002/08/22 15:42:10  tng
 * Remove unused parameter variables in inline functions.
 *
 * Revision 1.3  2002/07/11 18:21:20  knoaman
 * Grammar caching/preparsing - initial implementation.
 *
 * Revision 1.2  2002/07/05 17:08:10  tng
 * [Bug 10119] Grammar::getGrammarType need a const modifier
 *
 * Revision 1.1.1.1  2002/02/01 22:22:46  peiyongz
 * sane_include
 *
 * Revision 1.14  2001/11/19 18:26:31  knoaman
 * no message
 *
 * Revision 1.13  2001/10/09 12:18:26  tng
 * Leak fix: should use delete [] to delete fTargetNamespace.
 *
 * Revision 1.12  2001/10/04 15:11:51  knoaman
 * Add support for circular import.
 *
 * Revision 1.10  2001/09/14 14:50:22  tng
 * Schema: Fix some wildcard bugs, and some retrieving qualified/unqualified element decl problems.
 *
 * Revision 1.9  2001/08/21 16:06:11  tng
 * Schema: Unique Particle Attribution Constraint Checking.
 *
 * Revision 1.8  2001/07/31 15:27:10  knoaman
 * Added support for <attributeGroup>.
 *
 * Revision 1.7  2001/07/24 18:33:46  knoaman
 * Added support for <group> + extra constraint checking for complexType
 *
 * Revision 1.6  2001/06/25 12:51:57  knoaman
 * Add constraint checking on elements in complex types to prevent same
 * element names from having different definitions - use substitueGroups.
 *
 * Revision 1.5  2001/05/28 20:56:19  tng
 * Schema: Move getTargetNamespace as virtual function in base class Grammar
 *
 * Revision 1.4  2001/05/11 13:27:36  tng
 * Copyright update.
 *
 * Revision 1.3  2001/05/03 20:34:43  tng
 * Schema: SchemaValidator update
 *
 * Revision 1.2  2001/04/19 17:43:19  knoaman
 * More schema implementation classes.
 *
 * Revision 1.1  2001/03/21 21:56:33  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
 */



#if !defined(SCHEMAGRAMMAR_HPP)
#define SCHEMAGRAMMAR_HPP

#include <xercesc/framework/XMLNotationDecl.hpp>
#include <xercesc/util/RefHash3KeysIdPool.hpp>
#include <xercesc/util/NameIdPool.hpp>
#include <xercesc/util/StringPool.hpp>
#include <xercesc/validators/common/Grammar.hpp>
#include <xercesc/validators/schema/SchemaElementDecl.hpp>
#include <xercesc/util/ValueVectorOf.hpp>
#include <xercesc/validators/datatype/IDDatatypeValidator.hpp>
#include <xercesc/validators/datatype/DatatypeValidatorFactory.hpp>
#include <xercesc/framework/XMLSchemaDescription.hpp>
#include <xercesc/framework/ValidationContext.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
// This class stores the Schema information
//  NOTE: Schemas are not namespace aware, so we just use regular NameIdPool
//  data structures to store element and attribute decls. They are all set
//  to be in the global namespace and the full QName is used as the base name
//  of the decl. This means that all the URI parameters below are expected
//  to be null pointers (and anything else will cause an exception.)
//

// ---------------------------------------------------------------------------
//  Forward Declarations
// ---------------------------------------------------------------------------
class ComplexTypeInfo;
class NamespaceScope;
class XercesGroupInfo;
class XercesAttGroupInfo;
class XSAnnotation;

// ---------------------------------------------------------------------------
//  typedef declaration
// ---------------------------------------------------------------------------
typedef ValueVectorOf<SchemaElementDecl*> ElemVector;


class VALIDATORS_EXPORT SchemaGrammar : public Grammar
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    SchemaGrammar(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    virtual ~SchemaGrammar();

    // -----------------------------------------------------------------------
    //  Implementation of Virtual Interface
    // -----------------------------------------------------------------------
    virtual Grammar::GrammarType getGrammarType() const;
    virtual const XMLCh* getTargetNamespace() const;

    // this method should only be used while the grammar is being
    // constructed, not while it is being used
    // in a validation episode!
    virtual XMLElementDecl* findOrAddElemDecl
    (
        const   unsigned int    uriId
        , const XMLCh* const    baseName
        , const XMLCh* const    prefixName
        , const XMLCh* const    qName
        , unsigned int          scope
        ,       bool&           wasAdded
    ) ;

    virtual unsigned int getElemId
    (
        const   unsigned int    uriId
        , const XMLCh* const    baseName
        , const XMLCh* const    qName
        , unsigned int          scope
    )   const ;

    virtual const XMLElementDecl* getElemDecl
    (
        const   unsigned int    uriId
        , const XMLCh* const    baseName
        , const XMLCh* const    qName
        , unsigned int          scope
    )   const ;

    virtual XMLElementDecl* getElemDecl
    (
        const   unsigned int    uriId
        , const XMLCh* const    baseName
        , const XMLCh* const    qName
        , unsigned int          scope
    );

    virtual const XMLElementDecl* getElemDecl
    (
        const   unsigned int    elemId
    )   const;

    virtual XMLElementDecl* getElemDecl
    (
        const   unsigned int    elemId
    );

    virtual const XMLNotationDecl* getNotationDecl
    (
        const   XMLCh* const    notName
    )   const;

    virtual XMLNotationDecl* getNotationDecl
    (
        const   XMLCh* const    notName
    );

    virtual bool getValidated() const;

    virtual XMLElementDecl* putElemDecl
    (
        const   unsigned int    uriId
        , const XMLCh* const    baseName
        , const XMLCh* const    prefixName
        , const XMLCh* const    qName
        , unsigned int          scope
        , const bool            notDeclared = false
    );

    virtual unsigned int putElemDecl
    (
        XMLElementDecl* const elemDecl
        , const bool          notDeclared = false
    )   ;

    virtual unsigned int putNotationDecl
    (
        XMLNotationDecl* const notationDecl
    )   const;

    virtual void setValidated(const bool newState);

    virtual void reset();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    RefHash3KeysIdPoolEnumerator<SchemaElementDecl> getElemEnumerator() const;
    NameIdPoolEnumerator<XMLNotationDecl> getNotationEnumerator() const;
    RefHashTableOf<XMLAttDef>* getAttributeDeclRegistry() const;
    RefHashTableOf<ComplexTypeInfo>* getComplexTypeRegistry() const;
    RefHashTableOf<XercesGroupInfo>* getGroupInfoRegistry() const;
    RefHashTableOf<XercesAttGroupInfo>* getAttGroupInfoRegistry() const;
    DatatypeValidatorFactory* getDatatypeRegistry();
    NamespaceScope* getNamespaceScope() const;
    RefHash2KeysTableOf<ElemVector>* getValidSubstitutionGroups() const;

    //deprecated
    RefHashTableOf<XMLRefInfo>* getIDRefList() const;

    ValidationContext*          getValidationContext() const;

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setTargetNamespace(const XMLCh* const targetNamespace);
    void setAttributeDeclRegistry(RefHashTableOf<XMLAttDef>* const attReg);
    void setComplexTypeRegistry(RefHashTableOf<ComplexTypeInfo>* const other);
    void setGroupInfoRegistry(RefHashTableOf<XercesGroupInfo>* const other);
    void setAttGroupInfoRegistry(RefHashTableOf<XercesAttGroupInfo>* const other);
    void setNamespaceScope(NamespaceScope* const nsScope);
    void setValidSubstitutionGroups(RefHash2KeysTableOf<ElemVector>* const);

    virtual void                    setGrammarDescription( XMLGrammarDescription*);
    virtual XMLGrammarDescription*  getGrammarDescription() const;

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    unsigned int putGroupElemDecl
    (
        XMLElementDecl* const elemDecl
    )   const;

    // -----------------------------------------------------------------------
    // Annotation management methods
    // -----------------------------------------------------------------------
    /**
      * Add annotation to the list of annotations for a given key
      */
    void putAnnotation(void* key, XSAnnotation* const annotation);

    /**
      * Add global annotation
      *
      * Note: XSAnnotation acts as a linked list
      */
    void addAnnotation(XSAnnotation* const annotation);

    /**
     * Retrieve the annotation that is associated with the specified key
     *
     * @param  key   represents a schema component object (i.e. SchemaGrammar)
     * @return XSAnnotation associated with the key object
     */
    XSAnnotation* getAnnotation(const void* const key);

    /**
     * Retrieve the annotation that is associated with the specified key
     *
     * @param  key   represents a schema component object (i.e. SchemaGrammar)
     * @return XSAnnotation associated with the key object
     */
    const XSAnnotation* getAnnotation(const void* const key) const;

    /**
      * Get global annotation
      */
    XSAnnotation* getAnnotation();
    const XSAnnotation* getAnnotation() const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(SchemaGrammar)

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    SchemaGrammar(const SchemaGrammar&);
    SchemaGrammar& operator=(const SchemaGrammar&);

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    void cleanUp();

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fElemDeclPool
    //      This is the element decl pool. It contains all of the elements
    //      declared in the Schema (and their associated attributes.)
    //
    //  fElemNonDeclPool
    //      This is the element decl pool that is is populated as new elements
    //      are seen in the XML document (not declared in the Schema), and they
    //      are given default characteristics.
    //
    //  fGroupElemDeclPool
    //      This is the element decl pool for elements in a group that are
    //      referenced in different scope. It contains all of the elements
    //      declared in the Schema (and their associated attributes.)
    //
    //  fNotationDeclPool
    //      This is a pool of NotationDecl objects, which contains all of the
    //      notations declared in the Schema.
    //
    //  fTargetNamespace
    //      Target name space for this grammar.
    //
    //  fAttributeDeclRegistry
    //      Global attribute declarations
    //
    //  fComplexTypeRegistry
    //      Stores complexType declaration info
    //
    //  fGroupInfoRegistry
    //      Stores global <group> declaration info
    //
    //  fAttGroupInfoRegistry
    //      Stores global <attributeGroup> declaration info
    //
    //  fDatatypeRegistry
    //      Datatype validator factory
    //
    //  fNamespaceScope
    //      Prefix to Namespace map
    //
    //  fValidSubstitutionGroups
    //      Valid list of elements that can substitute a given element
    //
    //  fIDRefList
    //      List of ids of schema declarations extracted during schema grammar
    //      traversal
    //
    //  fValidated
    //      Indicates if the content of the Grammar has been pre-validated
    //      or not (UPA checking, etc.). When using a cached grammar, no need
    //      for pre content validation.
    //
    //  fGramDesc: adopted
    //
    // -----------------------------------------------------------------------
    XMLCh*                                 fTargetNamespace;
    RefHash3KeysIdPool<SchemaElementDecl>* fElemDeclPool;
    RefHash3KeysIdPool<SchemaElementDecl>* fElemNonDeclPool;
    RefHash3KeysIdPool<SchemaElementDecl>* fGroupElemDeclPool;
    NameIdPool<XMLNotationDecl>*           fNotationDeclPool;
    RefHashTableOf<XMLAttDef>*             fAttributeDeclRegistry;
    RefHashTableOf<ComplexTypeInfo>*       fComplexTypeRegistry;
    RefHashTableOf<XercesGroupInfo>*       fGroupInfoRegistry;
    RefHashTableOf<XercesAttGroupInfo>*    fAttGroupInfoRegistry;
    NamespaceScope*                        fNamespaceScope;
    RefHash2KeysTableOf<ElemVector>*       fValidSubstitutionGroups;
    ValidationContext*                     fValidationContext;
    MemoryManager*                         fMemoryManager;
    bool                                   fValidated;
    DatatypeValidatorFactory               fDatatypeRegistry;
    XMLSchemaDescription*                  fGramDesc;
    RefHashTableOf<XSAnnotation>*          fAnnotations;
};


// ---------------------------------------------------------------------------
//  SchemaGrammar: Getter methods
// ---------------------------------------------------------------------------
inline RefHash3KeysIdPoolEnumerator<SchemaElementDecl>
SchemaGrammar::getElemEnumerator() const
{
    return RefHash3KeysIdPoolEnumerator<SchemaElementDecl>(fElemDeclPool, false, fMemoryManager);
}

inline NameIdPoolEnumerator<XMLNotationDecl>
SchemaGrammar::getNotationEnumerator() const
{
    return NameIdPoolEnumerator<XMLNotationDecl>(fNotationDeclPool, fMemoryManager);
}

inline RefHashTableOf<XMLAttDef>* SchemaGrammar::getAttributeDeclRegistry() const {

    return fAttributeDeclRegistry;
}

inline RefHashTableOf<ComplexTypeInfo>*
SchemaGrammar::getComplexTypeRegistry() const {

    return fComplexTypeRegistry;
}

inline RefHashTableOf<XercesGroupInfo>*
SchemaGrammar::getGroupInfoRegistry() const {

    return fGroupInfoRegistry;
}

inline RefHashTableOf<XercesAttGroupInfo>*
SchemaGrammar::getAttGroupInfoRegistry() const {

    return fAttGroupInfoRegistry;
}

inline DatatypeValidatorFactory* SchemaGrammar::getDatatypeRegistry() {

    return &fDatatypeRegistry;
}

inline NamespaceScope* SchemaGrammar::getNamespaceScope() const {

    return fNamespaceScope;
}

inline RefHash2KeysTableOf<ElemVector>*
SchemaGrammar::getValidSubstitutionGroups() const {

    return fValidSubstitutionGroups;
}

inline RefHashTableOf<XMLRefInfo>* SchemaGrammar::getIDRefList() const {

    return fValidationContext->getIdRefList();
}

inline ValidationContext* SchemaGrammar::getValidationContext() const {

    return fValidationContext;
}

inline XMLGrammarDescription* SchemaGrammar::getGrammarDescription() const
{
    return fGramDesc;
}

inline XSAnnotation* SchemaGrammar::getAnnotation(const void* const key)
{
    return fAnnotations->get(key);
}

inline const XSAnnotation* SchemaGrammar::getAnnotation(const void* const key) const
{
    return fAnnotations->get(key);
}

inline XSAnnotation* SchemaGrammar::getAnnotation()
{
    return fAnnotations->get(this);
}

inline const XSAnnotation* SchemaGrammar::getAnnotation() const
{
    return fAnnotations->get(this);
}

// -----------------------------------------------------------------------
//  Setter methods
// -----------------------------------------------------------------------
inline void SchemaGrammar::setTargetNamespace(const XMLCh* const targetNamespace)
{
    if (fTargetNamespace)
        fMemoryManager->deallocate(fTargetNamespace);//delete [] fTargetNamespace;
    fTargetNamespace = XMLString::replicate(targetNamespace, fMemoryManager);
}

inline void
SchemaGrammar::setAttributeDeclRegistry(RefHashTableOf<XMLAttDef>* const attReg) {

    fAttributeDeclRegistry = attReg;
}

inline void
SchemaGrammar::setComplexTypeRegistry(RefHashTableOf<ComplexTypeInfo>* const other) {

    fComplexTypeRegistry = other;
}

inline void
SchemaGrammar::setGroupInfoRegistry(RefHashTableOf<XercesGroupInfo>* const other) {

    fGroupInfoRegistry = other;
}

inline void
SchemaGrammar::setAttGroupInfoRegistry(RefHashTableOf<XercesAttGroupInfo>* const other) {

    fAttGroupInfoRegistry = other;
}

inline void SchemaGrammar::setNamespaceScope(NamespaceScope* const nsScope) {

    fNamespaceScope = nsScope;
}

inline void
SchemaGrammar::setValidSubstitutionGroups(RefHash2KeysTableOf<ElemVector>* const other) {

    fValidSubstitutionGroups = other;
}


// ---------------------------------------------------------------------------
//  SchemaGrammar: Virtual methods
// ---------------------------------------------------------------------------
inline Grammar::GrammarType SchemaGrammar::getGrammarType() const {
    return Grammar::SchemaGrammarType;
}

inline const XMLCh* SchemaGrammar::getTargetNamespace() const {
    return fTargetNamespace;
}

// Element Decl
inline unsigned int SchemaGrammar::getElemId (const   unsigned int  uriId
                                              , const XMLCh* const    baseName
                                              , const XMLCh* const
                                              , unsigned int          scope ) const
{
    //
    //  In this case, we don't return zero to mean 'not found', so we have to
    //  map it to the official not found value if we don't find it.
    //
    const SchemaElementDecl* decl = fElemDeclPool->getByKey(baseName, uriId, scope);
    if (!decl) {

        decl = fGroupElemDeclPool->getByKey(baseName, uriId, scope);

        if (!decl)
            return XMLElementDecl::fgInvalidElemId;
    }
    return decl->getId();
}

inline const XMLElementDecl* SchemaGrammar::getElemDecl( const   unsigned int  uriId
                                              , const XMLCh* const    baseName
                                              , const XMLCh* const
                                              , unsigned int          scope )   const
{
    const SchemaElementDecl* decl = fElemDeclPool->getByKey(baseName, uriId, scope);

    if (!decl) {

        decl = fGroupElemDeclPool->getByKey(baseName, uriId, scope);

        if (!decl && fElemNonDeclPool)
            decl = fElemNonDeclPool->getByKey(baseName, uriId, scope);
    }

    return decl;
}

inline XMLElementDecl* SchemaGrammar::getElemDecl (const   unsigned int  uriId
                                              , const XMLCh* const    baseName
                                              , const XMLCh* const
                                              , unsigned int          scope )
{
    SchemaElementDecl* decl = fElemDeclPool->getByKey(baseName, uriId, scope);

    if (!decl) {

        decl = fGroupElemDeclPool->getByKey(baseName, uriId, scope);

        if (!decl && fElemNonDeclPool)
            decl = fElemNonDeclPool->getByKey(baseName, uriId, scope);
    }

    return decl;
}

inline const XMLElementDecl* SchemaGrammar::getElemDecl(const unsigned int elemId) const
{
    // Look up this element decl by id
    const SchemaElementDecl* decl = fElemDeclPool->getById(elemId);

    if (!decl)
        decl = fGroupElemDeclPool->getById(elemId);

    return decl;
}

inline XMLElementDecl* SchemaGrammar::getElemDecl(const unsigned int elemId)
{
    // Look up this element decl by id
    SchemaElementDecl* decl = fElemDeclPool->getById(elemId);

    if (!decl)
        decl = fGroupElemDeclPool->getById(elemId);

    return decl;
}

inline unsigned int
SchemaGrammar::putElemDecl(XMLElementDecl* const elemDecl,
                           const bool notDeclared) 
{
    if (notDeclared)
    {
        if(!fElemNonDeclPool)
            fElemNonDeclPool = new (fMemoryManager) RefHash3KeysIdPool<SchemaElementDecl>(29, true, 128, fMemoryManager);
        return fElemNonDeclPool->put(elemDecl->getBaseName(), elemDecl->getURI(), ((SchemaElementDecl* )elemDecl)->getEnclosingScope(), (SchemaElementDecl*) elemDecl);
    }

    return fElemDeclPool->put(elemDecl->getBaseName(), elemDecl->getURI(), ((SchemaElementDecl* )elemDecl)->getEnclosingScope(), (SchemaElementDecl*) elemDecl);
}

inline unsigned int SchemaGrammar::putGroupElemDecl (XMLElementDecl* const elemDecl)   const
{
    return fGroupElemDeclPool->put(elemDecl->getBaseName(), elemDecl->getURI(), ((SchemaElementDecl* )elemDecl)->getEnclosingScope(), (SchemaElementDecl*) elemDecl);
}

// Notation Decl
inline const XMLNotationDecl* SchemaGrammar::getNotationDecl(const XMLCh* const notName) const
{
    return fNotationDeclPool->getByKey(notName);
}

inline XMLNotationDecl* SchemaGrammar::getNotationDecl(const XMLCh* const notName)
{
    return fNotationDeclPool->getByKey(notName);
}

inline unsigned int SchemaGrammar::putNotationDecl(XMLNotationDecl* const notationDecl)   const
{
    return fNotationDeclPool->put(notationDecl);
}

inline bool SchemaGrammar::getValidated() const
{
    return fValidated;
}

inline void SchemaGrammar::setValidated(const bool newState)
{
    fValidated = newState;
}

XERCES_CPP_NAMESPACE_END

#endif
