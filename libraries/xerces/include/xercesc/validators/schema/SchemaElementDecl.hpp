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
 * $Log: SchemaElementDecl.hpp,v $
 * Revision 1.21  2004/02/05 18:08:38  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.20  2004/01/29 11:52:31  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.19  2003/12/24 17:42:03  knoaman
 * Misc. PSVI updates
 *
 * Revision 1.18  2003/12/12 18:36:37  peiyongz
 * getObjectType()
 *
 * Revision 1.17  2003/11/24 05:21:04  neilg
 * update method documentation
 *
 * Revision 1.16  2003/11/21 22:34:46  neilg
 * More schema component model implementation, thanks to David Cargill.
 * In particular, this cleans up and completes the XSModel, XSNamespaceItem,
 * XSAttributeDeclaration and XSAttributeGroup implementations.
 *
 * Revision 1.15  2003/11/06 15:30:08  neilg
 * first part of PSVI/schema component model implementation, thanks to David Cargill.  This covers setting the PSVIHandler on parser objects, as well as implementing XSNotation, XSSimpleTypeDefinition, XSIDCDefinition, and most of XSWildcard, XSComplexTypeDefinition, XSElementDeclaration, XSAttributeDeclaration and XSAttributeUse.
 *
 * Revision 1.14  2003/10/14 15:22:28  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.13  2003/10/05 02:08:05  neilg
 * Because it makes grammars un-sharable between parsers running on multiple threads, xsi:type should not be handled by modifying element declarations.  Modifying implementation so it no longer relies on this kind of behaviour; marking methods as deprecated which imply that xsi:type will be handled in this way.  Once their behaviour is handled elsewhere, these methods should eventually be removed
 *
 * Revision 1.12  2003/08/29 11:44:18  gareth
 * If a type was explicitly declared as anyType that now gets set in DOMTypeInfo. Added test cases.
 *
 * Revision 1.11  2003/05/18 14:02:08  knoaman
 * Memory manager implementation: pass per instance manager.
 *
 * Revision 1.10  2003/05/16 21:43:21  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.9  2003/05/15 18:57:27  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.8  2003/01/29 19:47:16  gareth
 * added DOMTypeInfo and some PSVI methods
 *
 * Revision 1.7  2002/11/04 14:49:41  tng
 * C++ Namespace Support.
 *
 * Revision 1.6  2002/07/12 15:17:48  knoaman
 * For a given global element, store info about a substitution group element
 * as a SchemaElementDecl and not as a string.
 *
 * Revision 1.5  2002/04/01 15:47:06  knoaman
 * Move Element Consistency checking (ref to global declarations) to SchemaValidator.
 *
 * Revision 1.4  2002/03/21 16:31:43  knoaman
 * Remove data/methods from SchemaElementDecl that are not used.
 *
 * Revision 1.3  2002/03/04 15:09:50  knoaman
 * Fix for bug 6834.
 *
 * Revision 1.2  2002/02/06 22:30:50  knoaman
 * Added a new attribute to store the wild card information for elements of type 'anyType'.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:46  peiyongz
 * sane_include
 *
 * Revision 1.16  2001/11/23 18:25:45  tng
 * Eliminate Warning from AIX xlC 3.6: 1540-399: (W) "IdentityConstraint" is undefined.  The delete operator will not call a destructor.
 *
 * Revision 1.15  2001/11/02 14:13:45  knoaman
 * Add support for identity constraints.
 *
 * Revision 1.14  2001/10/11 12:07:39  tng
 * Schema: model type should be based on complextypeinfo if exists.
 *
 * Revision 1.13  2001/09/05 20:49:11  knoaman
 * Fix for complexTypes with mixed content model.
 *
 * Revision 1.12  2001/08/29 20:52:35  tng
 * Schema: xsi:type support
 *
 * Revision 1.11  2001/08/21 16:06:11  tng
 * Schema: Unique Particle Attribution Constraint Checking.
 *
 * Revision 1.10  2001/08/09 15:23:16  knoaman
 * add support for <anyAttribute> declaration.
 *
 * Revision 1.9  2001/07/24 18:33:46  knoaman
 * Added support for <group> + extra constraint checking for complexType
 *
 * Revision 1.8  2001/06/13 20:51:15  peiyongz
 * fIsMixed: to handle mixed Content Model
 *
 * Revision 1.7  2001/05/11 13:27:36  tng
 * Copyright update.
 *
 * Revision 1.6  2001/05/03 20:34:43  tng
 * Schema: SchemaValidator update
 *
 * Revision 1.5  2001/05/03 19:18:01  knoaman
 * TraverseSchema Part II.
 *
 * Revision 1.4  2001/04/19 17:43:17  knoaman
 * More schema implementation classes.
 *
 * Revision 1.3  2001/03/21 21:56:33  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
 * Revision 1.2  2001/03/21 19:30:17  tng
 * Schema: Content Model Updates, by Pei Yong Zhang.
 *
 * Revision 1.1  2001/02/27 18:48:23  tng
 * Schema: Add SchemaAttDef, SchemaElementDecl, SchemaAttDefList.
 *
 */


#if !defined(SCHEMAELEMENTDECL_HPP)
#define SCHEMAELEMENTDECL_HPP

#include <xercesc/util/QName.hpp>
#include <xercesc/validators/common/Grammar.hpp>
#include <xercesc/validators/schema/ComplexTypeInfo.hpp>
#include <xercesc/validators/schema/identity/IdentityConstraint.hpp>
#include <xercesc/validators/datatype/DatatypeValidator.hpp>
#include <xercesc/validators/datatype/UnionDatatypeValidator.hpp>
#include <xercesc/validators/schema/PSVIDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class ContentSpecNode;
class SchemaAttDefList;

//
//  This class is a derivative of the basic element decl. This one implements
//  the virtuals so that they work for a Schema.
//
class VALIDATORS_EXPORT SchemaElementDecl : public XMLElementDecl
{
public :

    // -----------------------------------------------------------------------
    //  Class specific types
    //
    //  ModelTypes
    //      Indicates the type of content model that an element has. This
    //      indicates how the content model is represented and validated.
    // -----------------------------------------------------------------------
    enum ModelTypes
    {
        Empty
        , Any
        , Mixed_Simple
        , Mixed_Complex
        , Children
        , Simple

        , ModelTypes_Count
    };

    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    SchemaElementDecl(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    SchemaElementDecl
    (
          const XMLCh* const   prefix
        , const XMLCh* const   localPart
        , const int            uriId
        , const ModelTypes     modelType = Any
        , const int            enclosingScope = Grammar::TOP_LEVEL_SCOPE
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    SchemaElementDecl
    (
          const QName* const   elementName
        , const ModelTypes     modelType = Any
        , const int            enclosingScope = Grammar::TOP_LEVEL_SCOPE
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    ~SchemaElementDecl();


    // -----------------------------------------------------------------------
    //  The virtual element decl interface
    // -----------------------------------------------------------------------
    virtual XMLAttDef* findAttr
    (
        const   XMLCh* const    qName
        , const unsigned int    uriId
        , const XMLCh* const    baseName
        , const XMLCh* const    prefix
        , const LookupOpts      options
        ,       bool&           wasAdded
    )   const;
    virtual XMLAttDefList& getAttDefList() const;
    virtual CharDataOpts getCharDataOpts() const;
    virtual bool hasAttDefs() const;
    // @deprecated; not thread-safe
    virtual bool resetDefs();
    virtual const ContentSpecNode* getContentSpec() const;
    virtual ContentSpecNode* getContentSpec();
    virtual void setContentSpec(ContentSpecNode* toAdopt);
    virtual XMLContentModel* getContentModel();
    virtual void setContentModel(XMLContentModel* const newModelToAdopt);
    virtual const XMLCh* getFormattedContentModel ()   const;


    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    const SchemaAttDef* getAttDef(const XMLCh* const baseName, const int uriId) const;
    SchemaAttDef* getAttDef(const XMLCh* const baseName, const int uriId);
    const SchemaAttDef* getAttWildCard() const;
    SchemaAttDef* getAttWildCard();
    ModelTypes getModelType() const;
    PSVIDefs::PSVIScope getPSVIScope() const;
    DatatypeValidator* getDatatypeValidator() const;
    int getEnclosingScope() const;
    int getFinalSet() const;
    int getBlockSet() const;
    int getMiscFlags() const;
    XMLCh* getDefaultValue() const;
    ComplexTypeInfo* getComplexTypeInfo() const;
    virtual bool isGlobalDecl() const;
    SchemaElementDecl* getSubstitutionGroupElem() const;


    // ----------------------------------------------------------------------
    // Partial implementation of PSVI
    // The values these methods return are only accurate until the cleanUp method
    // is called (in the end tag part of the scanner you are using)
    // note that some of this information has dependancies. For example,
    // if something is not valid then the information returned by the other 
    // calls may be meaningless
    // See http://www.w3.org/TR/xmlschema-1/ for detailed information
    // ----------------------------------------------------------------------


    /**
     * The appropriate case among the following:
     * 1 If it was strictly assessed, then the appropriate case among the following:
     * 1.1 If all of the following are true
     *    1.1.1
     *    1.1.1.1 clause 1.1 of Schema-Validity Assessment (Element) (3.3.4) applied and the item was valid as defined by Element Locally Valid (Element) (3.3.4);
     *    1.1.1.2 clause 1.2 of Schema-Validity Assessment (Element) (3.3.4) applied and the item was valid as defined by Element Locally Valid (Type) (3.3.4).
     *    1.1.2 Neither its [children] nor its [attributes] contains an information item (element or attribute respectively) whose [validity] is invalid.
     *    1.1.3 Neither its [children] nor its [attributes] contains an information item (element or attribute respectively) with a context-determined declaration of mustFind whose [validity] is unknown.
     * , then valid;
     *    1.2 otherwise invalid.
     *    2 otherwise notKnown.
     * @deprecated; not thread-safe
     */
    PSVIDefs::Validity getValidity() const;


    /**
     * The appropriate case among the following:
     * 1 If it was strictly assessed and neither its [children] nor its [attributes] contains an information item (element or attribute respectively) whose [validation attempted] is not full, then full;
     * 2 If it was not strictly assessed and neither its [children] nor its [attributes] contains an information item (element or attribute respectively) whose [validation attempted] is not none, then none;
     *3 otherwise partial.
     * @deprecated; not thread-safe
     */
    PSVIDefs::Validation getValidationAttempted() const;


    /**
     * @return the complexity. simple or complex, depending on the type definition.
     * @deprecated; not thread-safe
     */
    PSVIDefs::Complexity getTypeType() const;

    /**
     * The target namespace of the type definition.
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    const XMLCh* getTypeUri() const;

    /**
     * The {name} of the type definition, if it is not absent. 
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    const XMLCh* getTypeName() const;

    /**
     * true if the {name} of the type definition is absent, otherwise false.
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    bool getTypeAnonymous() const;

    /**
     * If this method returns true and validity is VALID then the next three 
     * produce accurate results
     * @return true if the element is validated using a union type
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    bool isTypeDefinitionUnion() const;

    /**
     * The {target namespace} of the actual member type definition.
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    const XMLCh* getMemberTypeUri() const;

    /**
     * @return true if the {name} of the actual member type definition is absent, otherwise false.
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    bool getMemberTypeAnonymous() const;

    /**
     * @return the {name} of the actual member type definition, if it is not absent. 
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    const XMLCh* getMemberTypeName() const;


    /**
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    virtual const XMLCh* getDOMTypeInfoUri() const;
    /**
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    virtual const XMLCh* getDOMTypeInfoName() const;


    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    /**
     * @deprecated; not actually used
     */    
    void setElemId(unsigned int elemId);
    void setModelType(const SchemaElementDecl::ModelTypes toSet);
    void setPSVIScope(const PSVIDefs::PSVIScope toSet);
    void setDatatypeValidator(DatatypeValidator* newDatatypeValidator);
    void setEnclosingScope(const int enclosingScope);
    void setFinalSet(const int finalSet);
    void setBlockSet(const int blockSet);
    void setMiscFlags(const int flags);
    void setDefaultValue(const XMLCh* const value);
    void setComplexTypeInfo(ComplexTypeInfo* const typeInfo);
    /**
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    void setXsiComplexTypeInfo(ComplexTypeInfo* const typeInfo);
    /**
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    void setXsiSimpleTypeInfo(const DatatypeValidator* const dtv);
    void setAttWildCard(SchemaAttDef* const attWildCard);
    void setSubstitutionGroupElem(SchemaElementDecl* const elemDecl);
    /**
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    void setValidity(PSVIDefs::Validity valid);
    /**
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    void setValidationAttempted(PSVIDefs::Validation validation);
    
    /**
     * called when element content of this element was validated
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    void updateValidityFromElement(const XMLElementDecl *decl, Grammar::GrammarType eleGrammar);
    
    //called when attribute content of this element was validated    
    // @deprecated; should not be needed in a thread-safe implementation
    void updateValidityFromAttribute(const SchemaAttDef *def);

    /**
     * cleans up inbetween uses of the SchemaElementDecl. Resets xsiType, Validity etc.
     * @deprecated; not thread-safe (will not work with xsi:type and shared grammars)
     */
    void reset();

    // -----------------------------------------------------------------------
    //  IC methods
    // -----------------------------------------------------------------------
    void addIdentityConstraint(IdentityConstraint* const ic);
    unsigned int getIdentityConstraintCount() const;
    IdentityConstraint* getIdentityConstraintAt(unsigned int index) const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(SchemaElementDecl)

    virtual XMLElementDecl::objectType  getObjectType() const;

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    SchemaElementDecl(const SchemaElementDecl&);
    SchemaElementDecl& operator=(const SchemaElementDecl&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fModelType
    //      The content model type of this element. This tells us what kind
    //      of content model to create.
    //
    //  fDatatypeValidator
    //      The DatatypeValidator used to validate this element type.
    //
    //  fEnclosingScope
    //      The enclosing scope where this element is declared.
    //
    //  fFinalSet
    //      The value set of the 'final' attribute.
    //
    //  fBlockSet
    //      The value set of the 'block' attribute.
    //
    //  fMiscFlags
    //      Stores 'abstract/nullable' values
    //
    //  fDefaultValue
    //      The defalut/fixed value
    //
    //  fComplexTypeInfo
    //      Stores complex type information
    //      (no need to delete - handled by schema grammar)
    //
    //  fAttDefs
    //      The list of attributes that are faulted in for this element
    //      when ComplexTypeInfo does not exist.  We want to keep track
    //      of these faulted in attributes to avoid duplicate redundant
    //      error.
    //
    //  fXsiComplexTypeInfo
    //      Temporary store the xsi:type ComplexType here for validation
    //      If it presents, then it takes precedence than its own fComplexTypeInfo.
    //
    //  fXsiSimpleTypeInfo
    //      Temporary store the xsi:type SimpleType here for validation
    //      If it present then the information from it will be returned rather than fDatatypeValidator
    //
    //  fIdentityConstraints
    //      Store information about an element identity constraints.
    //
    //  fAttWildCard
    //      Store wildcard attribute in the case of an element with a type of
    //      'anyType'.
    //
    //  fSubstitutionGroupElem
    //      The substitution group element declaration.
    //
    //  fValidity
    //      After this attr has been validated this is its validity
    //
    //  fValidation
    //      The type of validation that happened to this attr
    //
    //  fSeenValidation
    //      set to true when a piece of content of this element is validated 
    //
    //  fSeenNoValidation
    //      set to true when a piece of content of this element is laxly or skip validated
    //
    //  fHadContent
    //      true when this element actually had content.
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    ModelTypes                         fModelType;
    PSVIDefs::PSVIScope                fPSVIScope;
    DatatypeValidator*                 fDatatypeValidator;
    int                                fEnclosingScope;
    int                                fFinalSet;
    int                                fBlockSet;
    int                                fMiscFlags;    
    XMLCh*                             fDefaultValue;
    ComplexTypeInfo*                   fComplexTypeInfo;
    RefHash2KeysTableOf<SchemaAttDef>* fAttDefs;
    ComplexTypeInfo*                   fXsiComplexTypeInfo;
    const DatatypeValidator*           fXsiSimpleTypeInfo;
    RefVectorOf<IdentityConstraint>*   fIdentityConstraints;
    SchemaAttDef*                      fAttWildCard;
    SchemaElementDecl*                 fSubstitutionGroupElem;
    PSVIDefs::Validity                 fValidity;
    PSVIDefs::Validation               fValidation;
    bool                               fSeenValidation;
    bool                               fSeenNoValidation;
    bool                               fHadContent;
};

// ---------------------------------------------------------------------------
//  SchemaElementDecl: XMLElementDecl virtual interface implementation
// ---------------------------------------------------------------------------
inline ContentSpecNode* SchemaElementDecl::getContentSpec()
{
    if (fComplexTypeInfo != 0) {
        return fComplexTypeInfo->getContentSpec();
    }

    return 0;
}

inline const ContentSpecNode* SchemaElementDecl::getContentSpec() const
{
    if (fComplexTypeInfo != 0) {
        return fComplexTypeInfo->getContentSpec();
    }

    return 0;
}

inline void
SchemaElementDecl::setContentSpec(ContentSpecNode*)
{
    //Handled by complexType
}

inline XMLContentModel* SchemaElementDecl::getContentModel()
{
    if (fComplexTypeInfo != 0) {
        return fComplexTypeInfo->getContentModel();
    }
    return 0;
}

inline void
SchemaElementDecl::setContentModel(XMLContentModel* const)
{
    //Handled by complexType
}


// ---------------------------------------------------------------------------
//  SchemaElementDecl: Getter methods
// ---------------------------------------------------------------------------
inline SchemaElementDecl::ModelTypes SchemaElementDecl::getModelType() const
{
    if (fComplexTypeInfo) {
        return (SchemaElementDecl::ModelTypes) fComplexTypeInfo->getContentType();
    }

    return fModelType;
}

inline PSVIDefs::PSVIScope SchemaElementDecl::getPSVIScope() const
{
    return fPSVIScope;
}

inline DatatypeValidator* SchemaElementDecl::getDatatypeValidator() const
{

    return fDatatypeValidator;
}

inline int SchemaElementDecl::getEnclosingScope() const
{
    return fEnclosingScope;
}

inline int SchemaElementDecl::getFinalSet() const
{
    return fFinalSet;
}

inline int SchemaElementDecl::getBlockSet() const
{
    return fBlockSet;
}

inline int SchemaElementDecl::getMiscFlags() const
{
    return fMiscFlags;
}

inline XMLCh* SchemaElementDecl::getDefaultValue() const
{
    return fDefaultValue;
}

inline ComplexTypeInfo* SchemaElementDecl::getComplexTypeInfo() const
{

    return fComplexTypeInfo;
}

inline const SchemaAttDef* SchemaElementDecl::getAttWildCard() const {

    return fAttWildCard;
}

inline SchemaAttDef* SchemaElementDecl::getAttWildCard() {

    return fAttWildCard;
}

inline bool SchemaElementDecl::isGlobalDecl() const {

    return (fEnclosingScope == Grammar::TOP_LEVEL_SCOPE);
}

inline SchemaElementDecl*
SchemaElementDecl::getSubstitutionGroupElem() const {

    return fSubstitutionGroupElem;
}

inline const XMLCh* SchemaElementDecl::getTypeName() const {
    // removing fXsi* references would break DOMTypeInfo implementation completely;
    // will have to wait for now
    if (fXsiComplexTypeInfo)
        return fXsiComplexTypeInfo->getTypeLocalName();
    else if (fComplexTypeInfo) 
        return fComplexTypeInfo->getTypeLocalName();
    else if(fXsiSimpleTypeInfo)
        return fXsiSimpleTypeInfo->getTypeLocalName();
    else if(fDatatypeValidator)
        return fDatatypeValidator->getTypeLocalName();

    //its anyType if we have not done validation on it or none of the above exist
    return SchemaSymbols::fgATTVAL_ANYTYPE;
}

inline PSVIDefs::Complexity SchemaElementDecl::getTypeType() const {
    if(getModelType() == Simple) {
        return PSVIDefs::SIMPLE;
    }
    else {
        return PSVIDefs::COMPLEX;
    }
}


inline const XMLCh* SchemaElementDecl::getTypeUri() const {
    // removing fXsi* references would break DOMTypeInfo implementation completely;
    // will have to wait for now
    if (fXsiComplexTypeInfo)
        return fXsiComplexTypeInfo->getTypeUri();
    else if (fComplexTypeInfo)
        return fComplexTypeInfo->getTypeUri();
    else if(fXsiSimpleTypeInfo)
        return fXsiSimpleTypeInfo->getTypeUri();
    else if(fDatatypeValidator)
        return fDatatypeValidator->getTypeUri();

    //its anyType if we have not done validation on it or none of the above exist
    return SchemaSymbols::fgURI_SCHEMAFORSCHEMA;
}

inline const XMLCh* SchemaElementDecl::getMemberTypeName() const {
    // removing fXsi* references would break DOMTypeInfo implementation completely;
    // will have to wait for now
    if(fXsiSimpleTypeInfo && fXsiSimpleTypeInfo->getType() == DatatypeValidator::Union)
        return ((UnionDatatypeValidator*)fXsiSimpleTypeInfo)->getMemberTypeName();
    else if(fDatatypeValidator && fDatatypeValidator->getType() == DatatypeValidator::Union)
        return ((UnionDatatypeValidator*)fDatatypeValidator)->getMemberTypeName();
    return 0;
}

inline const XMLCh* SchemaElementDecl::getMemberTypeUri() const {
    // removing fXsi* references would break DOMTypeInfo implementation completely;
    // will have to wait for now
    if(fXsiSimpleTypeInfo && fXsiSimpleTypeInfo->getType() == DatatypeValidator::Union)
        return ((UnionDatatypeValidator*)fXsiSimpleTypeInfo)->getMemberTypeUri();
    if(fDatatypeValidator && fDatatypeValidator->getType() == DatatypeValidator::Union)
        return ((UnionDatatypeValidator*)fDatatypeValidator)->getMemberTypeUri();

    return 0;
}

inline bool SchemaElementDecl::getMemberTypeAnonymous() const {
    // removing fXsi* references would break DOMTypeInfo implementation completely;
    // will have to wait for now
    if(fXsiSimpleTypeInfo && fXsiSimpleTypeInfo->getType() == DatatypeValidator::Union)
        return ((UnionDatatypeValidator*)fXsiSimpleTypeInfo)->getMemberTypeAnonymous();
    else if(fDatatypeValidator && fDatatypeValidator->getType() == DatatypeValidator::Union)
        return ((UnionDatatypeValidator*)fDatatypeValidator)->getMemberTypeAnonymous();
    return false;
}

inline bool SchemaElementDecl::isTypeDefinitionUnion() const {
    // removing fXsi* references would break DOMTypeInfo implementation completely;
    // will have to wait for now
   if(fXsiSimpleTypeInfo && fXsiSimpleTypeInfo->getType() == DatatypeValidator::Union ||
      fDatatypeValidator && fDatatypeValidator->getType() == DatatypeValidator::Union)
       return true;
    return false;
}

inline PSVIDefs::Validity SchemaElementDecl::getValidity() const {
    return fValidity;
}

inline PSVIDefs::Validation SchemaElementDecl::getValidationAttempted() const {
    if(!fHadContent)
        return fValidation;

    if(!fSeenNoValidation && fSeenValidation)
        return PSVIDefs::FULL;
    else if(fSeenNoValidation && !fSeenValidation)
        return PSVIDefs::NONE;
    else
        return PSVIDefs::PARTIAL;
}

inline bool SchemaElementDecl::getTypeAnonymous() const {
    
    //REVISIT - since xsi type have to be accessed through names 
    //presumeably they cannot be anonymous
    
    if (fXsiComplexTypeInfo) {
        return fXsiComplexTypeInfo->getAnonymous();
    }
    else if (fComplexTypeInfo) {
        return fComplexTypeInfo->getAnonymous();
    }
    else if(fXsiSimpleTypeInfo) {
        return fXsiSimpleTypeInfo->getAnonymous();
    }
    else if(fDatatypeValidator){
        return fDatatypeValidator->getAnonymous();
    }

    return false;
}

inline const XMLCh* SchemaElementDecl::getDOMTypeInfoName() const {
    // removing fXsi* references would break DOMTypeInfo implementation completely;
    // will have to wait for now
    if(fValidity != PSVIDefs::VALID) {
        if(getTypeType() == PSVIDefs::SIMPLE)
            return SchemaSymbols::fgDT_ANYSIMPLETYPE;
        else
            return SchemaSymbols::fgATTVAL_ANYTYPE;
    }

    if(getTypeAnonymous() || getMemberTypeAnonymous())
        return 0;
    if(fDatatypeValidator && fDatatypeValidator->getType() == DatatypeValidator::Union)
        return ((UnionDatatypeValidator*)fDatatypeValidator)->getMemberTypeName();
    if(fXsiSimpleTypeInfo && fXsiSimpleTypeInfo->getType() == DatatypeValidator::Union)
        return ((UnionDatatypeValidator*)fXsiSimpleTypeInfo)->getMemberTypeName();
    return getTypeName();
}

inline const XMLCh* SchemaElementDecl::getDOMTypeInfoUri() const {

    // removing fXsi* references would break DOMTypeInfo implementation completely;
    // will have to wait for now
    if(fValidity != PSVIDefs::VALID)
        return SchemaSymbols::fgURI_SCHEMAFORSCHEMA;

    if(getTypeAnonymous() || getMemberTypeAnonymous())
        return 0;

    if(fDatatypeValidator && fDatatypeValidator->getType() == DatatypeValidator::Union)
        return ((UnionDatatypeValidator*)fDatatypeValidator)->getMemberTypeUri();

    if(fXsiSimpleTypeInfo && fXsiSimpleTypeInfo->getType() == DatatypeValidator::Union)
        return ((UnionDatatypeValidator*)fXsiSimpleTypeInfo)->getMemberTypeUri();


    return getTypeUri();
}

// ---------------------------------------------------------------------------
//  SchemaElementDecl: Setter methods
// ---------------------------------------------------------------------------
inline void
SchemaElementDecl::setElemId(unsigned int)
{
    //there is not getElemId so this is not needed. mark deprecated.
    //fElemId = elemId;
}

inline void
SchemaElementDecl::setModelType(const SchemaElementDecl::ModelTypes toSet)
{
    fModelType = toSet;
}

inline void
SchemaElementDecl::setPSVIScope(const PSVIDefs::PSVIScope toSet)
{
    fPSVIScope = toSet;
}

inline void SchemaElementDecl::setDatatypeValidator(DatatypeValidator* newDatatypeValidator)
{
    fDatatypeValidator = newDatatypeValidator;
}

inline void SchemaElementDecl::setEnclosingScope(const int newEnclosingScope)
{
    fEnclosingScope = newEnclosingScope;
}

inline void SchemaElementDecl::setFinalSet(const int finalSet)
{
    fFinalSet = finalSet;
}

inline void SchemaElementDecl::setBlockSet(const int blockSet)
{
    fBlockSet = blockSet;
}

inline void SchemaElementDecl::setMiscFlags(const int flags)
{
    fMiscFlags = flags;
}

inline void SchemaElementDecl::setDefaultValue(const XMLCh* const value)
{
    if (fDefaultValue) {
        getMemoryManager()->deallocate(fDefaultValue);//delete[] fDefaultValue;
    }

    fDefaultValue = XMLString::replicate(value, getMemoryManager());
}

inline void
SchemaElementDecl::setComplexTypeInfo(ComplexTypeInfo* const typeInfo)
{
    fComplexTypeInfo = typeInfo;
}

inline void
SchemaElementDecl::setXsiComplexTypeInfo(ComplexTypeInfo* const typeInfo)
{
    fXsiComplexTypeInfo = typeInfo;
}

inline void
SchemaElementDecl::setXsiSimpleTypeInfo(const DatatypeValidator* const dtv)
{
    fXsiSimpleTypeInfo = dtv;
}

inline void
SchemaElementDecl::setAttWildCard(SchemaAttDef* const attWildCard) {

    if (fAttWildCard)
        delete fAttWildCard;

    fAttWildCard = attWildCard;
}

inline void
SchemaElementDecl::setSubstitutionGroupElem(SchemaElementDecl* const elemDecl) {

    fSubstitutionGroupElem = elemDecl;
}

inline void SchemaElementDecl::setValidity(PSVIDefs::Validity valid) {
    fValidity = valid;
}

inline void SchemaElementDecl::setValidationAttempted(PSVIDefs::Validation validation) {
    fValidation = validation;
}

inline void SchemaElementDecl::updateValidityFromAttribute(const SchemaAttDef *def) {

    PSVIDefs::Validation curValAttemted = def->getValidationAttempted();
    PSVIDefs::Validity curVal = def->getValidity();
        
    if(curValAttemted == PSVIDefs::NONE || curValAttemted == PSVIDefs::PARTIAL) {
        fSeenNoValidation = true;
        fValidity = PSVIDefs::UNKNOWN;
    }
    else {
        fSeenValidation = true;
    }
        
    if(curVal == PSVIDefs::INVALID)
        fValidity = PSVIDefs::INVALID;

    fHadContent = true;
}

inline void SchemaElementDecl::updateValidityFromElement(const XMLElementDecl *decl, Grammar::GrammarType eleGrammar) {

    if (eleGrammar == Grammar::SchemaGrammarType) {                    
        PSVIDefs::Validation curValAttemted = ((SchemaElementDecl *)decl)->getValidationAttempted();
        PSVIDefs::Validity curVal = ((SchemaElementDecl *)decl)->getValidity();
        
        if(curValAttemted == PSVIDefs::NONE || curValAttemted == PSVIDefs::PARTIAL) {
            fSeenNoValidation = true;
            fValidity = PSVIDefs::UNKNOWN;
        }
        else {
            fSeenValidation = true;
        }
        
        if(curVal == PSVIDefs::INVALID)
            fValidity = PSVIDefs::INVALID;
    }

    fHadContent = true;

}

inline void SchemaElementDecl::reset() {
    if(fXsiSimpleTypeInfo && fXsiSimpleTypeInfo->getType() == DatatypeValidator::Union)
        ((UnionDatatypeValidator *)fXsiSimpleTypeInfo)->reset();
    if(fDatatypeValidator && fDatatypeValidator->getType() == DatatypeValidator::Union)
        ((UnionDatatypeValidator *)fDatatypeValidator)->reset();

    setXsiSimpleTypeInfo(0);
    setXsiComplexTypeInfo(0);
    fValidity = PSVIDefs::UNKNOWN;
    fValidation = PSVIDefs::NONE;    
    fSeenValidation = false;
    fSeenNoValidation = false;
    fHadContent = false;
}

// ---------------------------------------------------------------------------
//  SchemaElementDecl: IC methods
// ---------------------------------------------------------------------------
inline void
SchemaElementDecl::addIdentityConstraint(IdentityConstraint* const ic) {

    if (!fIdentityConstraints) {
        fIdentityConstraints = new (getMemoryManager()) RefVectorOf<IdentityConstraint>(16, true, getMemoryManager());
    }

    fIdentityConstraints->addElement(ic);
}

inline unsigned int SchemaElementDecl::getIdentityConstraintCount() const {

    if (fIdentityConstraints) {
        return fIdentityConstraints->size();
    }

    return 0;
}

inline IdentityConstraint*
SchemaElementDecl::getIdentityConstraintAt(unsigned int index) const {

    if (fIdentityConstraints) {
        return fIdentityConstraints->elementAt(index);
    }

    return 0;
}

XERCES_CPP_NAMESPACE_END

#endif
