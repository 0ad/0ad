/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Log: IGXMLScanner.hpp,v $
 * Revision 1.18  2004/01/29 11:46:30  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.17  2003/11/28 19:54:31  knoaman
 * PSVIElement update
 *
 * Revision 1.16  2003/11/28 05:13:29  neilg
 * Fix state-ful duplicate attribute detection when the integrated
 * scanner is in use and namespaces are off.  Also, implement
 * change to PSVIHandler interface to remove prefix passing.
 *
 * Revision 1.15  2003/11/27 22:52:37  knoaman
 * PSVIElement implementation
 *
 * Revision 1.14  2003/11/27 06:10:31  neilg
 * PSVIAttribute implementation
 *
 * Revision 1.13  2003/11/26 16:20:00  knoaman
 * Store XSModel.
 *
 * Revision 1.12  2003/11/24 05:09:38  neilg
 * implement new, statless, method for detecting duplicate attributes
 *
 * Revision 1.11  2003/10/22 20:22:30  knoaman
 * Prepare for annotation support.
 *
 * Revision 1.10  2003/09/22 19:51:41  neilg
 * scanners should maintain their own pools of undeclared elements, rather than requiring grammars to do this.  This makes grammar objects stateless with regard to validation.
 *
 * Revision 1.9  2003/08/14 02:56:41  knoaman
 * Code refactoring to improve performance of validation.
 *
 * Revision 1.8  2003/07/10 19:47:23  peiyongz
 * Stateless Grammar: Initialize scanner with grammarResolver,
 *                                creating grammar through grammarPool
 *
 * Revision 1.7  2003/05/22 02:10:51  knoaman
 * Default the memory manager.
 *
 * Revision 1.6  2003/05/15 18:26:29  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.5  2003/03/07 18:08:58  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.4  2003/01/29 19:59:35  gareth
 * we now detect when elements and attributes are validated/ the result of the validation and store that information.
 *
 * Revision 1.3  2003/01/15 15:49:49  knoaman
 * Change constant declaration name to match its value.
 *
 * Revision 1.2  2003/01/13 16:30:18  knoaman
 * [Bug 14469] Validator doesn't enforce xsd:key.
 *
 * Revision 1.1  2002/12/04 02:05:25  knoaman
 * Initial checkin.
 *
 */


#if !defined(IGXMLSCANNER_HPP)
#define IGXMLSCANNER_HPP

#include <xercesc/internal/XMLScanner.hpp>
#include <xercesc/util/KVStringPair.hpp>
#include <xercesc/util/NameIdPool.hpp>
#include <xercesc/util/RefHash3KeysIdPool.hpp>
#include <xercesc/validators/common/Grammar.hpp>
#include <xercesc/validators/schema/SchemaElementDecl.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class DTDElementDecl;
class DTDGrammar;
class DTDValidator;
class SchemaValidator;
class ValueStoreCache;
class XPathMatcherStack;
class FieldActivator;
class IdentityConstraint;
class ContentLeafNameTypeVector;
class SchemaAttDef;
class XMLContentModel;
class XSModel;
class PSVIAttributeList;
class PSVIElement;

//  This is an integrated scanner class, which does DTD/XML Schema grammar
//  processing.
class XMLPARSER_EXPORT IGXMLScanner : public XMLScanner
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    IGXMLScanner
    (
        XMLValidator* const valToAdopt
        , GrammarResolver* const grammarResolver
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    IGXMLScanner
    (
          XMLDocumentHandler* const docHandler
        , DocTypeHandler* const     docTypeHandler
        , XMLEntityHandler* const   entityHandler
        , XMLErrorReporter* const   errReporter
        , XMLValidator* const       valToAdopt
        , GrammarResolver* const    grammarResolver
        , MemoryManager* const      manager = XMLPlatformUtils::fgMemoryManager
    );
    virtual ~IGXMLScanner();

    // -----------------------------------------------------------------------
    //  XMLScanner public virtual methods
    // -----------------------------------------------------------------------
    virtual const XMLCh* getName() const;
    virtual NameIdPool<DTDEntityDecl>* getEntityDeclPool();
    virtual const NameIdPool<DTDEntityDecl>* getEntityDeclPool() const;
    virtual unsigned int resolveQName
    (
        const   XMLCh* const        qName
        ,       XMLBuffer&          prefixBufToFill
        , const short               mode
        ,       int&                prefixColonPos
    );
    virtual void scanDocument
    (
        const   InputSource&    src
    );
    virtual bool scanNext(XMLPScanToken& toFill);
    virtual Grammar* loadGrammar
    (
        const   InputSource&    src
        , const short           grammarType
        , const bool            toCache = false
    );

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    IGXMLScanner();
    IGXMLScanner(const IGXMLScanner&);
    IGXMLScanner& operator=(const IGXMLScanner&);

    // -----------------------------------------------------------------------
    //  XMLScanner virtual methods
    // -----------------------------------------------------------------------
    virtual void scanCDSection();
    virtual void scanCharData(XMLBuffer& toToUse);
    virtual EntityExpRes scanEntityRef
    (
        const   bool    inAttVal
        ,       XMLCh&  firstCh
        ,       XMLCh&  secondCh
        ,       bool&   escaped
    );
    virtual void scanDocTypeDecl();
    virtual void scanReset(const InputSource& src);
    virtual void sendCharData(XMLBuffer& toSend);

    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------
    void commonInit();
    void cleanUp();
    InputSource* resolveSystemId(const XMLCh* const sysId); // return owned by caller

    // Spaces are not allowed in URI, so %20 is used instead.
    // Convert %20 to spaces before resolving the URI
    void normalizeURI(const XMLCh* const systemURI, XMLBuffer& normalizedURI);

    unsigned int buildAttList
    (
        const   RefVectorOf<KVStringPair>&  providedAttrs
        , const unsigned int                attCount
        ,       XMLElementDecl*             elemDecl
        ,       RefVectorOf<XMLAttr>&       toFill
    );
    bool normalizeAttValue
    (
        const   XMLAttDef* const    attDef
        , const XMLCh* const       name 
        , const XMLCh* const        value
        ,       XMLBuffer&          toFill
    );
    bool normalizeAttRawValue
    (
        const   XMLCh* const        attrName
        , const XMLCh* const        value
        ,       XMLBuffer&          toFill
    );
    unsigned int resolvePrefix
    (
        const   XMLCh* const        prefix
        , const ElemStack::MapModes mode
    );
    unsigned int resolvePrefix
    (
        const   XMLCh* const        prefix
        ,       XMLBuffer&          uriBufToFill
        , const ElemStack::MapModes mode
    );
    void updateNSMap
    (
        const   XMLCh* const    attrName
        , const XMLCh* const    attrValue
    );
    void scanRawAttrListforNameSpaces(int attCount);
    void parseSchemaLocation(const XMLCh* const schemaLocationStr);
    void resolveSchemaGrammar(const XMLCh* const loc, const XMLCh* const uri);
    bool switchGrammar(const XMLCh* const newGrammarNameSpace);
    bool laxElementValidation(QName* element, ContentLeafNameTypeVector* cv,
                              const XMLContentModel* const cm,
                              const unsigned int parentElemDepth);
    bool anyAttributeValidation(SchemaAttDef* attWildCard,
                                unsigned int uriId,
                                bool& skipThisOne,
                                bool& laxThisOne);
    void resizeElemState();
    void processSchemaLocation(XMLCh* const schemaLoc);

    // -----------------------------------------------------------------------
    //  Private scanning methods
    // -----------------------------------------------------------------------
    bool basicAttrValueScan
    (
        const   XMLCh* const    attrName
        ,       XMLBuffer&      toFill
    );
    unsigned int rawAttrScan
    (
        const   XMLCh* const                elemName
        ,       RefVectorOf<KVStringPair>&  toFill
        ,       bool&                       isEmpty
    );
    bool scanAttValue
    (
        const   XMLAttDef* const    attDef
        , const   XMLCh* const      attrName
        ,       XMLBuffer&          toFill
    );
    bool scanContent();
    void scanEndTag(bool& gotData);
    bool scanStartTag(bool& gotData);
    bool scanStartTagNS(bool& gotData);

    // -----------------------------------------------------------------------
    //  IdentityConstraints Activation methods
    // -----------------------------------------------------------------------
    void activateSelectorFor(IdentityConstraint* const ic, const int initialDepth);

    // -----------------------------------------------------------------------
    //  Grammar preparsing methods
    // -----------------------------------------------------------------------
    Grammar* loadXMLSchemaGrammar(const InputSource& src, const bool toCache = false);
    Grammar* loadDTDGrammar(const InputSource& src, const bool toCache = false);

    // -----------------------------------------------------------------------
    //  PSVI handling methods
    // -----------------------------------------------------------------------
    void endElementPSVI(SchemaElementDecl* const elemDecl,
                        DatatypeValidator* const memberDV);
    void resetPSVIElemContext();

    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fRawAttrList
    //      During the initial scan of the attributes we can only do a raw
    //      scan for key/value pairs. So this vector is used to store them
    //      until they can be processed (and put into fAttrList.)
    //
    //  fDTDValidator
    //      The DTD validator instance.
    //
    //  fSchemaValidator
    //      The Schema validator instance.
    //
    //  fSeeXsi
    //      This flag indicates a schema has been seen.
    //
    //  fElemState
    //  fElemStateSize
    //      Stores an element next state from DFA content model - used for
    //      wildcard validation
    //
    //  fMatcherStack
    //      Stack of active XPath matchers for identity constraints. All
    //      active XPath matchers are notified of startElement, characters
    //      and endElement callbacks in order to perform their matches.
    //
    //  fValueStoreCache
    //      Cache of value stores for identity constraint fields.
    //
    //  fFieldActivator
    //      Activates fields within a certain scope when a selector matches
    //      its xpath.
    // fDTDElemNonDeclPool
    //      registry of "faulted-in" DTD element decls
    // fSchemaElemNonDeclPool
    //      registry for elements without decls in the grammar
    // fElemCount
    //      count of the number of start tags seen so far (starts at 1).
    //      Used for duplicate attribute detection/processing of required/defaulted attributes
    // fAttDefRegistry
    //      mapping from XMLAttDef instances to the count of the last
    //      start tag where they were utilized.
    // fUndeclaredAttrRegistry
    //      mapping of attr QNames to the count of the last start tag in which they occurred
    // fUndeclaredAttrRegistryNS
    //      mapping of namespaceId/localName pairs to the count of the last
    //      start tag in which they occurred.
    //  fPSVIAttrList
    //      PSVI attribute list implementation that needs to be
    //      filled when a PSVIHandler is registered
    //
    // -----------------------------------------------------------------------
    bool                        fSeeXsi;
    Grammar::GrammarType        fGrammarType;
    unsigned int                fElemStateSize;
    unsigned int*               fElemState;
    XMLBuffer                   fContent;
    RefVectorOf<KVStringPair>*  fRawAttrList;
    DTDValidator*               fDTDValidator;
    SchemaValidator*            fSchemaValidator;
    DTDGrammar*                 fDTDGrammar;
    XPathMatcherStack*          fMatcherStack;
    ValueStoreCache*            fValueStoreCache;
    FieldActivator*             fFieldActivator;
    ValueVectorOf<XMLCh*>*      fLocationPairs;
    NameIdPool<DTDElementDecl>* fDTDElemNonDeclPool;
    RefHash3KeysIdPool<SchemaElementDecl>* fSchemaElemNonDeclPool;
    unsigned int                            fElemCount;
    RefHashTableOf<unsigned int>*           fAttDefRegistry;
    RefHashTableOf<unsigned int>*           fUndeclaredAttrRegistry;
    RefHash2KeysTableOf<unsigned int>*      fUndeclaredAttrRegistryNS;
    PSVIAttributeList *                     fPSVIAttrList;
    XSModel*                                fModel;
    PSVIElement*                            fPSVIElement;
    ValueStackOf<bool>*                     fErrorStack;
    PSVIElemContext                         fPSVIElemContext;
};

inline const XMLCh* IGXMLScanner::getName() const
{
    return XMLUni::fgIGXMLScanner;
}


XERCES_CPP_NAMESPACE_END

#endif
