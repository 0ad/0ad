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
 * $Log: SchemaValidator.hpp,v $
 * Revision 1.25  2004/01/29 11:52:31  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.24  2003/12/03 20:00:27  neilg
 * PSVI fix:  cannot allow validator to reset its element content buffer before exposing it to the application
 *
 * Revision 1.23  2003/11/28 21:18:32  knoaman
 * Make use of canonical representation in PSVIElement
 *
 * Revision 1.22  2003/11/27 22:52:37  knoaman
 * PSVIElement implementation
 *
 * Revision 1.21  2003/11/27 06:10:31  neilg
 * PSVIAttribute implementation
 *
 * Revision 1.20  2003/11/24 05:13:20  neilg
 * expose validator that actually validated attribute.  Clean up union handling
 *
 * Revision 1.19  2003/10/05 02:09:37  neilg
 * the validator now keeps track of the current complex and simple type (including if this is an xsi:type).  This allows both the validator and the scanner to know what the current type is, without the need to modify the element declaration each time an xsi:type is seen
 *
 * Revision 1.18  2003/08/14 03:01:04  knoaman
 * Code refactoring to improve performance of validation.
 *
 * Revision 1.17  2003/05/18 14:02:08  knoaman
 * Memory manager implementation: pass per instance manager.
 *
 * Revision 1.16  2003/05/16 21:43:21  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.15  2003/05/16 03:15:51  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.14  2003/05/15 18:57:27  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.13  2003/01/20 19:04:48  knoaman
 * Fix for particle derivation checking.
 *
 * Revision 1.12  2003/01/13 20:16:51  knoaman
 * [Bug 16024] SchemaSymbols.hpp conflicts C++ Builder 6 dir.h
 *
 * Revision 1.11  2003/01/09 22:34:54  tng
 * [Bug 14955] error validating parser.
 *
 * Revision 1.10  2002/11/07 21:57:37  tng
 * Fix the following Schema Test Failures:
 * 1. Typo when comparing miscFlags with FIXED
 * 2. If xsi:type is specified, need to validate using that xsitype validator even if the type was any
 * 3. Need to check ID/IDREFs for element value
 * 4. Need to duplicate attribute id for wildcard scenario.
 *
 * Revision 1.9  2002/11/04 14:49:42  tng
 * C++ Namespace Support.
 *
 * Revision 1.8  2002/09/04 18:17:41  tng
 * Do not set IDREF to used during prevalidation.
 *
 * Revision 1.7  2002/07/11 18:55:45  knoaman
 * Add a flag to the preContentValidation method to indicate whether to validate
 * default/fixed attributes or not.
 *
 * Revision 1.6  2002/06/17 18:09:29  tng
 * DOM L3: support "datatype-normalization"
 *
 * Revision 1.5  2002/05/22 20:54:14  knoaman
 * Prepare for DOM L3 :
 * - Make use of the XMLEntityHandler/XMLErrorReporter interfaces, instead of using
 * EntityHandler/ErrorHandler directly.
 * - Add 'AbstractDOMParser' class to provide common functionality for XercesDOMParser
 * and DOMBuilder.
 *
 * Revision 1.4  2002/04/19 13:33:23  knoaman
 * Fix for bug 8236.
 *
 * Revision 1.3  2002/04/01 15:47:06  knoaman
 * Move Element Consistency checking (ref to global declarations) to SchemaValidator.
 *
 * Revision 1.2  2002/03/25 20:25:32  knoaman
 * Move particle derivation checking from TraverseSchema to SchemaValidator.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:47  peiyongz
 * sane_include
 *
 * Revision 1.8  2001/11/13 13:25:08  tng
 * Deprecate function XMLValidator::checkRootElement.
 *
 * Revision 1.7  2001/06/05 16:51:21  knoaman
 * Add 'const' to getGrammar - submitted by Peter A. Volchek.
 *
 * Revision 1.6  2001/05/11 15:17:48  tng
 * Schema: Nillable fixes.
 *
 * Revision 1.5  2001/05/11 13:27:37  tng
 * Copyright update.
 *
 * Revision 1.4  2001/05/03 20:34:45  tng
 * Schema: SchemaValidator update
 *
 * Revision 1.3  2001/04/19 18:17:40  tng
 * Schema: SchemaValidator update, and use QName in Content Model
 *
 * Revision 1.2  2001/03/30 16:35:20  tng
 * Schema: Whitespace normalization.
 *
 * Revision 1.1  2001/03/21 21:56:33  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
 */



#if !defined(SCHEMAVALIDATOR_HPP)
#define SCHEMAVALIDATOR_HPP

#include <xercesc/framework/XMLValidator.hpp>
#include <xercesc/framework/XMLBuffer.hpp>
#include <xercesc/util/ValueStackOf.hpp>
#include <xercesc/validators/common/ContentSpecNode.hpp>
#include <xercesc/validators/schema/SchemaGrammar.hpp>
#include <xercesc/validators/schema/XSDErrorReporter.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class GrammarResolver;
class DatatypeValidator;
class SchemaElementDecl;

//
//  This is a derivative of the abstract validator interface. This class
//  implements a validator that supports standard XML Schema semantics.
//  This class handles scanning the of the schema, and provides
//  the standard validation services against the Schema info it found.
//
class VALIDATORS_EXPORT SchemaValidator : public XMLValidator
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    SchemaValidator
    (
          XMLErrorReporter* const errReporter = 0
          , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );
    virtual ~SchemaValidator();

    // -----------------------------------------------------------------------
    //  Implementation of the XMLValidator interface
    // -----------------------------------------------------------------------
    virtual int checkContent
    (
        XMLElementDecl* const   elemDecl
        , QName** const         children
        , const unsigned int    childCount
    );

    virtual void faultInAttr
    (
                XMLAttr&    toFill
        , const XMLAttDef&  attDef
    )   const;

    virtual void preContentValidation(bool reuseGrammar,
                                      bool validateDefAttr = false);

    virtual void postParseValidation();

    virtual void reset();

    virtual bool requiresNamespaces() const;

    virtual void validateAttrValue
    (
        const   XMLAttDef*                  attDef
        , const XMLCh* const                attrValue
        , bool                              preValidation = false
        , const XMLElementDecl*             elemDecl = 0
    );

    virtual void validateElement
    (
        const   XMLElementDecl*             elemDef
    );

    virtual Grammar* getGrammar() const;
    virtual void setGrammar(Grammar* aGrammar);

    // -----------------------------------------------------------------------
    //  Virtual DTD handler interface.
    // -----------------------------------------------------------------------
    virtual bool handlesDTD() const;

    // -----------------------------------------------------------------------
    //  Virtual Schema handler interface. handlesSchema() always return false.
    // -----------------------------------------------------------------------
    virtual bool handlesSchema() const;

    // -----------------------------------------------------------------------
    //  Schema Validator methods
    // -----------------------------------------------------------------------
    void normalizeWhiteSpace(DatatypeValidator* dV, const XMLCh* const value, XMLBuffer& toFill);

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setGrammarResolver(GrammarResolver* grammarResolver);

    void setXsiType(const XMLCh* const        prefix
      , const XMLCh* const        localPart
       , const unsigned int        uriId);

    void setNillable(bool isNil);
    void setErrorReporter(XMLErrorReporter* const errorReporter);
    void setExitOnFirstFatal(const bool newValue);
    void setDatatypeBuffer(const XMLCh* const value);
    void clearDatatypeBuffer();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    ComplexTypeInfo* getCurrentTypeInfo() const;
    DatatypeValidator *getCurrentDatatypeValidator() const;
    DatatypeValidator *getMostRecentAttrValidator() const;
    bool getErrorOccurred() const;
    bool getIsElemSpecified() const;
    const XMLCh* getNormalizedValue() const;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    SchemaValidator(const SchemaValidator&);
    SchemaValidator& operator=(const SchemaValidator&);

    // -----------------------------------------------------------------------
    //  Element Consitency Checking methods
    // -----------------------------------------------------------------------
    void checkRefElementConsistency(SchemaGrammar* const currentGrammar,
                                    const ComplexTypeInfo* const curTypeInfo,
                                    const XercesGroupInfo* const curGroup = 0);

    // -----------------------------------------------------------------------
    //  Particle Derivation Checking methods
    // -----------------------------------------------------------------------
    void checkParticleDerivation(SchemaGrammar* const currentGrammar,
                                 const ComplexTypeInfo* const typeInfo);
    void checkParticleDerivationOk(SchemaGrammar* const currentGrammar,
                                   ContentSpecNode* const curNode,
                                   const int derivedScope,
                                   ContentSpecNode* const baseNode,
                                   const int baseScope,
                                   const ComplexTypeInfo* const baseInfo = 0,
                                   const bool toCheckOccurrence = true);
    ContentSpecNode* checkForPointlessOccurrences(ContentSpecNode* const specNode,
                                                  const ContentSpecNode::NodeTypes nodeType,
                                                  ValueVectorOf<ContentSpecNode*>* const nodes);
    void gatherChildren(const ContentSpecNode::NodeTypes parentNodeType,
                        ContentSpecNode* const specNode,
                        ValueVectorOf<ContentSpecNode*>* const nodes);
    bool isOccurrenceRangeOK(const int min1, const int max1, const int min2, const int max2);
    void checkNSCompat(const ContentSpecNode* const derivedSpecNode,
                       const ContentSpecNode* const baseSpecNode,
                       const bool toCheckOccurence);
    bool wildcardEltAllowsNamespace(const ContentSpecNode* const baseSpecNode,
                                    const unsigned int derivedURI);
    void checkNameAndTypeOK(SchemaGrammar* const currentGrammar,
                            const ContentSpecNode* const derivedSpecNode,
                            const int derivedScope,
                            const ContentSpecNode* const baseSpecNode,
                            const int baseScope,
                            const ComplexTypeInfo* const baseInfo = 0);
    SchemaElementDecl* findElement(const int scope,
                                   const unsigned int uriIndex,
                                   const XMLCh* const name,
                                   SchemaGrammar* const grammar,
                                   const ComplexTypeInfo* const typeInfo = 0);
    void checkICRestriction(const SchemaElementDecl* const derivedElemDecl,
                            const SchemaElementDecl* const baseElemDecl,
                            const XMLCh* const derivedElemName,
                            const XMLCh* const baseElemName);
    void checkTypesOK(const SchemaElementDecl* const derivedElemDecl,
                      const SchemaElementDecl* const baseElemDecl,
                      const XMLCh* const derivedElemName);
    void checkRecurseAsIfGroup(SchemaGrammar* const currentGrammar,
                               ContentSpecNode* const derivedSpecNode,
                               const int derivedScope,
                               const ContentSpecNode* const baseSpecNode,
                               const int baseScope,
                               ValueVectorOf<ContentSpecNode*>* const nodes,
                               const ComplexTypeInfo* const baseInfo);
    void checkRecurse(SchemaGrammar* const currentGrammar,
                      const ContentSpecNode* const derivedSpecNode,
                      const int derivedScope,
                      ValueVectorOf<ContentSpecNode*>* const derivedNodes,
                      const ContentSpecNode* const baseSpecNode,
                      const int baseScope,
                      ValueVectorOf<ContentSpecNode*>* const baseNodes,
                      const ComplexTypeInfo* const baseInfo,
                      const bool toLax = false);
    void checkNSSubset(const ContentSpecNode* const derivedSpecNode,
                       const ContentSpecNode* const baseSpecNode);
    bool isWildCardEltSubset(const ContentSpecNode* const derivedSpecNode,
                             const ContentSpecNode* const baseSpecNode);
    void checkNSRecurseCheckCardinality(SchemaGrammar* const currentGrammar,
                                        const ContentSpecNode* const derivedSpecNode,
                                        ValueVectorOf<ContentSpecNode*>* const derivedNodes,
                                        const int derivedScope,
                                        ContentSpecNode* const baseSpecNode,
                                        const bool toCheckOccurence);
    void checkRecurseUnordered(SchemaGrammar* const currentGrammar,
                               const ContentSpecNode* const derivedSpecNode,
                               ValueVectorOf<ContentSpecNode*>* const derivedNodes,
                               const int derivedScope,
                               ContentSpecNode* const baseSpecNode,
                               ValueVectorOf<ContentSpecNode*>* const baseNodes,
                               const int baseScope,
                               const ComplexTypeInfo* const baseInfo);
    void checkMapAndSum(SchemaGrammar* const currentGrammar,
                        const ContentSpecNode* const derivedSpecNode,
                        ValueVectorOf<ContentSpecNode*>* const derivedNodes,
                        const int derivedScope,
                        ContentSpecNode* const baseSpecNode,
                        ValueVectorOf<ContentSpecNode*>* const baseNodes,
                        const int baseScope,
                        const ComplexTypeInfo* const baseInfo);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    // -----------------------------------------------------------------------
    //  The following comes from or set by the Scanner
    //  fSchemaGrammar
    //      The current schema grammar used by the validator
    //
    //  fGrammarResolver
    //      All the schema grammar stored
    //
    //  fXsiType
    //      Store the Schema Type Attribute Value if schema type is specified
    //
    //  fNil
    //      Indicates if Nillable has been set
    // -----------------------------------------------------------------------
    //  The following used internally in the validator
    //
    //  fCurrentDatatypeValidator
    //      The validator used for validating the content of elements
    //      with simple types
    //
    //  fDatatypeBuffer
    //      Buffer for simple type element string content
    //
    //  fTrailing
    //      Previous chunk had a trailing space
    //
    //  fSeenId
    //      Indicate if an attribute of ID type has been seen already, reset per element.
    //
    //  fSchemaErrorReporter
    //      Report schema process errors
    //
    //  fTypeStack
    //      Stack of complex type declarations.
    //
    //  fMostRecentAttrValidator
    //      DatatypeValidator that validated attribute most recently processed
    //
    //  fErrorOccurred
    //      whether an error occurred in the most recent operation
    // -----------------------------------------------------------------------
    MemoryManager*                  fMemoryManager;
    SchemaGrammar*                  fSchemaGrammar;
    GrammarResolver*                fGrammarResolver;
    QName*                          fXsiType;
    bool                            fNil;
    DatatypeValidator*              fCurrentDatatypeValidator;
    XMLBuffer*                      fNotationBuf;
    XMLBuffer                       fDatatypeBuffer;
    bool                            fTrailing;
    bool                            fSeenId;
    XSDErrorReporter                fSchemaErrorReporter;
    ValueStackOf<ComplexTypeInfo*>* fTypeStack;
    DatatypeValidator *             fMostRecentAttrValidator;
    bool                            fErrorOccurred;
    bool                            fElemIsSpecified;
};


// ---------------------------------------------------------------------------
//  SchemaValidator: Setter methods
// ---------------------------------------------------------------------------
inline void SchemaValidator::setGrammarResolver(GrammarResolver* grammarResolver) {
    fGrammarResolver = grammarResolver;
}

inline void SchemaValidator::setXsiType(const XMLCh* const        prefix
      , const XMLCh* const        localPart
       , const unsigned int        uriId)
{
    delete fXsiType;
    fXsiType = new (fMemoryManager) QName(prefix, localPart, uriId, fMemoryManager);
}

inline void SchemaValidator::setNillable(bool isNil) {
    fNil = isNil;
}

inline void SchemaValidator::setExitOnFirstFatal(const bool newValue) {

    fSchemaErrorReporter.setExitOnFirstFatal(newValue);
}

inline void SchemaValidator::setDatatypeBuffer(const XMLCh* const value)
{
    fDatatypeBuffer.append(value);
}

inline void SchemaValidator::clearDatatypeBuffer()
{
    fDatatypeBuffer.reset();
}

// ---------------------------------------------------------------------------
//  SchemaValidator: Getter methods
// ---------------------------------------------------------------------------
inline ComplexTypeInfo* SchemaValidator::getCurrentTypeInfo() const {
    if (fTypeStack->empty())
        return 0;
    return fTypeStack->peek();
}

inline DatatypeValidator * SchemaValidator::getCurrentDatatypeValidator() const 
{
    return fCurrentDatatypeValidator;
}
inline DatatypeValidator *SchemaValidator::getMostRecentAttrValidator() const
{
    return fMostRecentAttrValidator;
}

// ---------------------------------------------------------------------------
//  Virtual interface
// ---------------------------------------------------------------------------
inline Grammar* SchemaValidator::getGrammar() const {
    return fSchemaGrammar;
}

inline void SchemaValidator::setGrammar(Grammar* aGrammar) {
    fSchemaGrammar = (SchemaGrammar*) aGrammar;
}

inline void SchemaValidator::setErrorReporter(XMLErrorReporter* const errorReporter) {

    XMLValidator::setErrorReporter(errorReporter);
    fSchemaErrorReporter.setErrorReporter(errorReporter);
}

// ---------------------------------------------------------------------------
//  SchemaValidator: DTD handler interface
// ---------------------------------------------------------------------------
inline bool SchemaValidator::handlesDTD() const
{
    // No DTD scanning
    return false;
}

// ---------------------------------------------------------------------------
//  SchemaValidator: Schema handler interface
// ---------------------------------------------------------------------------
inline bool SchemaValidator::handlesSchema() const
{
    return true;
}

// ---------------------------------------------------------------------------
//  SchemaValidator: Particle derivation checking
// ---------------------------------------------------------------------------
inline bool
SchemaValidator::isOccurrenceRangeOK(const int min1, const int max1,
                                     const int min2, const int max2) {

    if (min1 >= min2 &&
        (max2 == SchemaSymbols::XSD_UNBOUNDED ||
         (max1 != SchemaSymbols::XSD_UNBOUNDED && max1 <= max2))) {
        return true;
    }
    return false;
}

inline bool SchemaValidator::getErrorOccurred() const
{
    return fErrorOccurred;
}

inline bool SchemaValidator::getIsElemSpecified() const
{
    return fElemIsSpecified;
}

inline const XMLCh* SchemaValidator::getNormalizedValue() const
{
    return fDatatypeBuffer.getRawBuffer();
}

XERCES_CPP_NAMESPACE_END

#endif
