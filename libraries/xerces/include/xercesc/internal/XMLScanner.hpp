/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2002 The Apache Software Foundation.  All rights
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
 * $Log: XMLScanner.hpp,v $
 * Revision 1.32  2003/12/31 15:40:00  cargilld
 * Release memory when an error is encountered.
 *
 * Revision 1.31  2003/11/28 21:18:32  knoaman
 * Make use of canonical representation in PSVIElement
 *
 * Revision 1.30  2003/11/28 19:54:31  knoaman
 * PSVIElement update
 *
 * Revision 1.29  2003/11/27 22:52:37  knoaman
 * PSVIElement implementation
 *
 * Revision 1.28  2003/11/24 05:09:38  neilg
 * implement new, statless, method for detecting duplicate attributes
 *
 * Revision 1.27  2003/11/13 15:00:44  peiyongz
 * Solve Compilation/Linkage error on AIX/Solaris/HP/Linux
 *
 * Revision 1.26  2003/11/12 20:29:47  peiyongz
 * Stateless Grammar: ValidationContext
 *
 * Revision 1.25  2003/11/06 15:30:06  neilg
 * first part of PSVI/schema component model implementation, thanks to David Cargill.  This covers setting the PSVIHandler on parser objects, as well as implementing XSNotation, XSSimpleTypeDefinition, XSIDCDefinition, and most of XSWildcard, XSComplexTypeDefinition, XSElementDeclaration, XSAttributeDeclaration and XSAttributeUse.
 *
 * Revision 1.24  2003/10/22 20:22:30  knoaman
 * Prepare for annotation support.
 *
 * Revision 1.23  2003/07/10 19:47:24  peiyongz
 * Stateless Grammar: Initialize scanner with grammarResolver,
 *                                creating grammar through grammarPool
 *
 * Revision 1.22  2003/05/16 21:36:58  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.21  2003/05/15 18:26:29  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.20  2003/04/22 14:52:37  knoaman
 * Initialize security manager in constructor.
 *
 * Revision 1.19  2003/04/17 22:00:46  neilg
 * This commit implements detection of exponential entity
 * expansions inside the scanner code.  This is only done when a
 * security manager instance has been registered with the parser by
 * the application.  The default number of entities which may be
 * expanded is 50000; this appears to work very well for SAX, but DOM
 * parsing applications may wish to set this limit considerably lower.
 *
 * Added SecurityManager to enable detection of exponentially-expanding entities
 * 
 * Revision 1.18  2003/03/10 15:27:29  tng
 * XML1.0 Errata E38
 *
 * Revision 1.17  2003/03/07 18:08:58  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.16  2003/01/03 20:08:40  tng
 * New feature StandardUriConformant to force strict standard uri conformance.
 *
 * Revision 1.15  2002/12/27 16:16:51  knoaman
 * Set scanner options and handlers.
 *
 * Revision 1.14  2002/12/20 22:09:56  tng
 * XML 1.1
 *
 * Revision 1.13  2002/12/04 01:41:14  knoaman
 * Scanner re-organization.
 *
 * Revision 1.12  2002/11/04 14:58:19  tng
 * C++ Namespace Support.
 *
 * Revision 1.11  2002/08/27 05:56:39  knoaman
 * Identity Constraint: handle case of recursive elements.
 *
 * Revision 1.10  2002/08/16 15:46:17  knoaman
 * Bug 7698 : filenames with embedded spaces in schemaLocation strings not handled properly.
 *
 * Revision 1.9  2002/07/31 18:49:29  tng
 * [Bug 6227] Make method getLastExtLocation() constant.
 *
 * Revision 1.8  2002/07/11 18:22:13  knoaman
 * Grammar caching/preparsing - initial implementation.
 *
 * Revision 1.7  2002/06/17 16:13:01  tng
 * DOM L3: Add the flag fNormalizeData so that datatype normalization defined by schema is done only if asked.
 *
 * Revision 1.6  2002/06/07 18:35:49  tng
 * Add getReaderMgr in XMLScanner so that the parser can query encoding information.
 *
 * Revision 1.5  2002/05/30 16:20:57  tng
 * Add feature to optionally ignore external DTD.
 *
 * Revision 1.4  2002/05/27 18:42:14  tng
 * To get ready for 64 bit large file, use XMLSSize_t to represent line and column number.
 *
 * Revision 1.3  2002/05/22 20:54:33  knoaman
 * Prepare for DOM L3 :
 * - Make use of the XMLEntityHandler/XMLErrorReporter interfaces, instead of using
 * EntityHandler/ErrorHandler directly.
 * - Add 'AbstractDOMParser' class to provide common functionality for XercesDOMParser
 * and DOMBuilder.
 *
 * Revision 1.2  2002/03/25 20:25:32  knoaman
 * Move particle derivation checking from TraverseSchema to SchemaValidator.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:03  peiyongz
 * sane_include
 *
 * Revision 1.38  2001/11/30 22:19:15  peiyongz
 * cleanUp function made member function
 * cleanUp object moved to file scope
 *
 * Revision 1.37  2001/11/20 18:51:44  tng
 * Schema: schemaLocation and noNamespaceSchemaLocation to be specified outside the instance document.  New methods setExternalSchemaLocation and setExternalNoNamespaceSchemaLocation are added (for SAX2, two new properties are added).
 *
 * Revision 1.36  2001/11/13 13:27:28  tng
 * Move root element check to XMLScanner.
 *
 * Revision 1.35  2001/11/02 14:20:14  knoaman
 * Add support for identity constraints.
 *
 * Revision 1.34  2001/10/12 20:52:18  tng
 * Schema: Find the attributes see if they should be (un)qualified.
 *
 * Revision 1.33  2001/09/10 15:16:04  tng
 * Store the fGrammarType instead of calling getGrammarType all the time for faster performance.
 *
 * Revision 1.32  2001/09/10 14:06:22  tng
 * Schema: AnyAttribute support in Scanner and Validator.
 *
 * Revision 1.31  2001/08/13 15:06:39  knoaman
 * update <any> validation.
 *
 * Revision 1.30  2001/08/02 16:54:39  tng
 * Reset some Scanner flags in scanReset().
 *
 * Revision 1.29  2001/08/01 19:11:01  tng
 * Add full schema constraint checking flag to the samples and the parser.
 *
 * Revision 1.28  2001/07/24 21:23:39  tng
 * Schema: Use DatatypeValidator for ID/IDREF/ENTITY/ENTITIES/NOTATION.
 *
 * Revision 1.27  2001/07/13 16:56:48  tng
 * ScanId fix.
 *
 * Revision 1.26  2001/07/12 18:50:17  tng
 * Some performance modification regarding standalone check and xml decl check.
 *
 * Revision 1.25  2001/07/10 21:09:31  tng
 * Give proper error messsage when scanning external id.
 *
 * Revision 1.24  2001/07/09 13:42:08  tng
 * Partial Markup in Parameter Entity is validity constraint and thus should be just error, not fatal error.
 *
 * Revision 1.23  2001/07/05 13:12:11  tng
 * Standalone checking is validity constraint and thus should be just error, not fatal error:
 *
 * Revision 1.22  2001/06/22 12:42:33  tng
 * [Bug 2257] 1.5 thinks a <?xml-stylesheet ...> tag is a <?xml ...> tag
 *
 * Revision 1.21  2001/06/04 20:59:29  jberry
 * Add method incrementErrorCount for use by validator. Make sure to reset error count in _both_ the scanReset methods.
 *
 * Revision 1.20  2001/06/03 19:21:40  jberry
 * Add support for tracking error count during parse; enables simple parse without requiring error handler.
 *
 * Revision 1.19  2001/05/28 20:55:02  tng
 * Schema: allocate a fDTDValidator, fSchemaValidator explicitly to avoid wrong cast
 *
 * Revision 1.18  2001/05/11 15:17:28  tng
 * Schema: Nillable fixes.
 *
 * Revision 1.17  2001/05/11 13:26:17  tng
 * Copyright update.
 *
 * Revision 1.16  2001/05/03 20:34:29  tng
 * Schema: SchemaValidator update
 *
 * Revision 1.15  2001/05/03 19:09:09  knoaman
 * Support Warning/Error/FatalError messaging.
 * Validity constraints errors are treated as errors, with the ability by user to set
 * validity constraints as fatal errors.
 *
 * Revision 1.14  2001/04/19 18:16:59  tng
 * Schema: SchemaValidator update, and use QName in Content Model
 *
 * Revision 1.13  2001/03/30 16:46:56  tng
 * Schema: Use setDoSchema instead of setSchemaValidation which makes more sense.
 *
 * Revision 1.12  2001/03/30 16:35:06  tng
 * Schema: Whitespace normalization.
 *
 * Revision 1.11  2001/03/21 21:56:05  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
 * Revision 1.10  2001/02/15 15:56:27  tng
 * Schema: Add setSchemaValidation and getSchemaValidation for DOMParser and SAXParser.
 * Add feature "http://apache.org/xml/features/validation/schema" for SAX2XMLReader.
 * New data field  fSchemaValidation in XMLScanner as the flag.
 *
 * Revision 1.9  2000/04/12 22:58:28  roddey
 * Added support for 'auto validate' mode.
 *
 * Revision 1.8  2000/03/03 01:29:32  roddey
 * Added a scanReset()/parseReset() method to the scanner and
 * parsers, to allow for reset after early exit from a progressive parse.
 * Added calls to new Terminate() call to all of the samples. Improved
 * documentation in SAX and DOM parsers.
 *
 * Revision 1.7  2000/03/02 19:54:30  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.6  2000/02/24 20:18:07  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.5  2000/02/06 07:47:54  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.4  2000/01/24 20:40:43  roddey
 * Exposed the APIs to get to the byte offset in the source XML buffer. This stuff
 * is not tested yet, but I wanted to get the API changes in now so that the API
 * can be stablized.
 *
 * Revision 1.3  2000/01/12 23:52:46  roddey
 * These are trivial changes required to get the C++ and Java versions
 * of error messages more into sync. Mostly it was where the Java version
 * was passing out one or more parameter than the C++ version was. In
 * some cases the change just required an extra parameter to get the
 * needed info to the place where the error was issued.
 *
 * Revision 1.2  2000/01/12 00:15:04  roddey
 * Changes to deal with multiply nested, relative pathed, entities and to deal
 * with the new URL class changes.
 *
 * Revision 1.1.1.1  1999/11/09 01:08:23  twl
 * Initial checkin
 *
 * Revision 1.4  1999/11/08 20:44:52  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(XMLSCANNER_HPP)
#define XMLSCANNER_HPP

#include <xercesc/framework/XMLBufferMgr.hpp>
#include <xercesc/framework/XMLErrorCodes.hpp>
#include <xercesc/framework/XMLRefInfo.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/NameIdPool.hpp>
#include <xercesc/util/RefHashTableOf.hpp>
#include <xercesc/util/SecurityManager.hpp>
#include <xercesc/internal/ReaderMgr.hpp>
#include <xercesc/internal/ElemStack.hpp>
#include <xercesc/validators/DTD/DTDEntityDecl.hpp>
#include <xercesc/framework/XMLAttr.hpp>
#include <xercesc/framework/ValidationContext.hpp>
#include <xercesc/validators/common/GrammarResolver.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class InputSource;
class XMLDocumentHandler;
class XMLEntityHandler;
class ErrorHandler;
class DocTypeHandler;
class XMLPScanToken;
class XMLStringPool;
class Grammar;
class XMLValidator;
class MemoryManager;
class PSVIHandler;


struct PSVIElemContext
{
    bool               fIsSpecified;
    bool               fErrorOccurred;
    int                fElemDepth;
    int                fFullValidationDepth;
    int                fNoneValidationDepth;
    DatatypeValidator* fCurrentDV;
    ComplexTypeInfo*   fCurrentTypeInfo;
    const XMLCh*       fNormalizedValue;
};

//  This is the mondo scanner class, which does the vast majority of the
//  work of parsing. It handles reading in input and spitting out events
//  to installed handlers.
class XMLPARSER_EXPORT XMLScanner : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Public class types
    //
    //  NOTE: These should really be private, but some of the compilers we
    //  have to deal with are too stupid to understand this.
    //
    //  DeclTypes
    //      Used by scanXMLDecl() to know what type of decl it should scan.
    //      Text decls have slightly different rules from XMLDecls.
    //
    //  EntityExpRes
    //      These are the values returned from the entity expansion method,
    //      to indicate how it went.
    //
    //  XMLTokens
    //      These represent the possible types of input we can get while
    //      scanning content.
    //
    //  ValScheme
    //      This indicates what the scanner should do in terms of validation.
    //      'Auto' means if there is any int/ext subset, then validate. Else,
    //      don't.
    // -----------------------------------------------------------------------
    enum DeclTypes
    {
        Decl_Text
        , Decl_XML
    };

    enum EntityExpRes
    {
        EntityExp_Pushed
        , EntityExp_Returned
        , EntityExp_Failed
    };

    enum XMLTokens
    {
        Token_CData
        , Token_CharData
        , Token_Comment
        , Token_EndTag
        , Token_EOF
        , Token_PI
        , Token_StartTag
        , Token_Unknown
    };

    enum ValSchemes
    {
        Val_Never
        , Val_Always
        , Val_Auto
    };


    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    XMLScanner
    (
        XMLValidator* const valToAdopt
        , GrammarResolver* const grammarResolver
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    XMLScanner
    (
        XMLDocumentHandler* const  docHandler
        , DocTypeHandler* const    docTypeHandler
        , XMLEntityHandler* const  entityHandler
        , XMLErrorReporter* const  errReporter
        , XMLValidator* const      valToAdopt
        , GrammarResolver* const grammarResolver
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    virtual ~XMLScanner();


    // -----------------------------------------------------------------------
    //  Error emitter methods
    // -----------------------------------------------------------------------
    bool emitErrorWillThrowException(const XMLErrs::Codes toEmit);
    void emitError(const XMLErrs::Codes toEmit);
    void emitError
    (
        const   XMLErrs::Codes    toEmit
        , const XMLCh* const        text1
        , const XMLCh* const        text2 = 0
        , const XMLCh* const        text3 = 0
        , const XMLCh* const        text4 = 0
    );
    void emitError
    (
        const   XMLErrs::Codes    toEmit
        , const char* const         text1
        , const char* const         text2 = 0
        , const char* const         text3 = 0
        , const char* const         text4 = 0
    );

    // -----------------------------------------------------------------------
    //  Public pure virtual methods
    // -----------------------------------------------------------------------
    virtual const XMLCh* getName() const = 0;
    virtual NameIdPool<DTDEntityDecl>* getEntityDeclPool() = 0;
    virtual const NameIdPool<DTDEntityDecl>* getEntityDeclPool() const = 0;
    virtual unsigned int resolveQName
    (
        const   XMLCh* const        qName
        ,       XMLBuffer&          prefixBufToFill
        , const short               mode
        ,       int&                prefixColonPos
    ) = 0;
    virtual void scanDocument
    (
        const   InputSource&    src
    ) = 0;
    virtual bool scanNext(XMLPScanToken& toFill) = 0;
    virtual Grammar* loadGrammar
    (
        const   InputSource&    src
        , const short           grammarType
        , const bool            toCache = false
    ) = 0;

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    const XMLDocumentHandler* getDocHandler() const;
    XMLDocumentHandler* getDocHandler();
    const DocTypeHandler* getDocTypeHandler() const;
    DocTypeHandler* getDocTypeHandler();
    bool getDoNamespaces() const;
    ValSchemes getValidationScheme() const;
    bool getDoSchema() const;
    bool getValidationSchemaFullChecking() const;
    const XMLEntityHandler* getEntityHandler() const;
    XMLEntityHandler* getEntityHandler();
    const XMLErrorReporter* getErrorReporter() const;
    XMLErrorReporter* getErrorReporter();
    const ErrorHandler* getErrorHandler() const;
    ErrorHandler* getErrorHandler();
    const PSVIHandler* getPSVIHandler() const;
    PSVIHandler* getPSVIHandler();
    bool getExitOnFirstFatal() const;
    bool getValidationConstraintFatal() const;
    RefHashTableOf<XMLRefInfo>* getIDRefList();
    const RefHashTableOf<XMLRefInfo>* getIDRefList() const;

    ValidationContext*   getValidationContext();

    bool getInException() const;
    /*bool getLastExtLocation
    (
                XMLCh* const    sysIdToFill
        , const unsigned int    maxSysIdChars
        ,       XMLCh* const    pubIdToFill
        , const unsigned int    maxPubIdChars
        ,       XMLSSize_t&     lineToFill
        ,       XMLSSize_t&     colToFill
    ) const;*/
    const Locator* getLocator() const;
    const ReaderMgr* getReaderMgr() const;
    unsigned int getSrcOffset() const;
    bool getStandalone() const;
    const XMLValidator* getValidator() const;
    XMLValidator* getValidator();
    int getErrorCount();
    const XMLStringPool* getURIStringPool() const;
    XMLStringPool* getURIStringPool();
    bool getHasNoDTD() const;
    XMLCh* getExternalSchemaLocation() const;
    XMLCh* getExternalNoNamespaceSchemaLocation() const;
    SecurityManager* getSecurityManager() const;
    bool getLoadExternalDTD() const;
    bool getNormalizeData() const;
    bool isCachingGrammarFromParse() const;
    bool isUsingCachedGrammarInParse() const;
    bool getCalculateSrcOfs() const;
    Grammar* getRootGrammar() const;
    XMLReader::XMLVersion getXMLVersion() const;
    MemoryManager* getMemoryManager() const;
    ValueVectorOf<PrefMapElem*>* getNamespaceContext() const;
    unsigned int getPrefixId(const XMLCh* const prefix) const;
    const XMLCh* getPrefixForId(unsigned int prefId) const;

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /**
      * When an attribute name has no prefix, unlike elements, it is not mapped
      * to the global namespace. So, in order to have something to map it to
      * for practical purposes, a id for an empty URL is created and used for
      * such names.
      *
      * @return The URL pool id of the URL for an empty URL "".
      */
    unsigned int getEmptyNamespaceId() const;

    /**
      * When a prefix is found that has not been mapped, an error is issued.
      * However, if the parser has been instructed not to stop on the first
      * fatal error, it needs to be able to continue. To do so, it will map
      * that prefix tot his magic unknown namespace id.
      *
      * @return The URL pool id of the URL for the unknown prefix
      *         namespace.
      */
    unsigned int getUnknownNamespaceId() const;

    /**
      * The prefix 'xml' is a magic prefix, defined by the XML spec and
      * requiring no prior definition. This method returns the id for the
      * intrinsically defined URL for this prefix.
      *
      * @return The URL pool id of the URL for the 'xml' prefix.
      */
    unsigned int getXMLNamespaceId() const;

    /**
      * The prefix 'xmlns' is a magic prefix, defined by the namespace spec
      * and requiring no prior definition. This method returns the id for the
      * intrinsically defined URL for this prefix.
      *
      * @return The URL pool id of the URL for the 'xmlns' prefix.
      */
    unsigned int getXMLNSNamespaceId() const;

    /**
      * This method find the passed URI id in its URI pool and
      * copy the text of that URI into the passed buffer.
      */
    bool getURIText
    (
        const   unsigned int    uriId
        ,       XMLBuffer&      uriBufToFill
    )   const;

    const XMLCh* getURIText(const   unsigned int    uriId) const;

    /* tell if the validator comes from user */
    bool isValidatorFromUser();

    /* tell if standard URI are forced */
    bool getStandardUriConformant() const;

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setDocHandler(XMLDocumentHandler* const docHandler);
    void setDocTypeHandler(DocTypeHandler* const docTypeHandler);
    void setDoNamespaces(const bool doNamespaces);
    void setEntityHandler(XMLEntityHandler* const docTypeHandler);
    void setErrorReporter(XMLErrorReporter* const errHandler);
    void setErrorHandler(ErrorHandler* const handler);
    void setPSVIHandler(PSVIHandler* const handler);
    void setURIStringPool(XMLStringPool* const stringPool);
    void setExitOnFirstFatal(const bool newValue);
    void setValidationConstraintFatal(const bool newValue);
    void setValidationScheme(const ValSchemes newScheme);
    void setValidator(XMLValidator* const valToAdopt);
    void setDoSchema(const bool doSchema);
    void setValidationSchemaFullChecking(const bool schemaFullChecking);
    void setHasNoDTD(const bool hasNoDTD);
    void cacheGrammarFromParse(const bool newValue);
    void useCachedGrammarInParse(const bool newValue);
    void setRootElemName(XMLCh* rootElemName);
    void setExternalSchemaLocation(const XMLCh* const schemaLocation);
    void setExternalNoNamespaceSchemaLocation(const XMLCh* const noNamespaceSchemaLocation);
    void setExternalSchemaLocation(const char* const schemaLocation);
    void setExternalNoNamespaceSchemaLocation(const char* const noNamespaceSchemaLocation);
    void setSecurityManager(SecurityManager* const securityManager);
    void setLoadExternalDTD(const bool loadDTD);
    void setNormalizeData(const bool normalizeData);
    void setCalculateSrcOfs(const bool newValue);
    void setParseSettings(XMLScanner* const refScanner);
    void setStandardUriConformant(const bool newValue);

    // -----------------------------------------------------------------------
    //  Mutator methods
    // -----------------------------------------------------------------------
    void incrementErrorCount(void);			// For use by XMLValidator

    // -----------------------------------------------------------------------
    //  Deprecated methods as of 3.2.0. Use getValidationScheme() and
    //  setValidationScheme() instead.
    // -----------------------------------------------------------------------
    bool getDoValidation() const;
    void setDoValidation(const bool validate);

    // -----------------------------------------------------------------------
    //  Document scanning methods
    //
    //  scanDocument() does the entire source document. scanFirst(),
    //  scanNext(), and scanReset() support a progressive parse.
    // -----------------------------------------------------------------------
    void scanDocument
    (
        const   XMLCh* const    systemId
    );
    void scanDocument
    (
        const   char* const     systemId
    );

    bool scanFirst
    (
        const   InputSource&    src
        ,       XMLPScanToken&  toFill
    );
    bool scanFirst
    (
        const   XMLCh* const    systemId
        ,       XMLPScanToken&  toFill
    );
    bool scanFirst
    (
        const   char* const     systemId
        ,       XMLPScanToken&  toFill
    );

    void scanReset(XMLPScanToken& toFill);

    bool checkXMLDecl(bool startWithAngle);

    // -----------------------------------------------------------------------
    //  Grammar preparsing methods
    // -----------------------------------------------------------------------
    Grammar* loadGrammar
    (
        const   XMLCh* const    systemId
        , const short           grammarType
        , const bool            toCache = false
    );
    Grammar* loadGrammar
    (
        const   char* const     systemId
        , const short           grammarType
        , const bool            toCache = false
    );

    // -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
	static void reinitScannerMutex();
	static void reinitMsgLoader();

protected:
    // -----------------------------------------------------------------------
    //  Protected pure virtual methods
    // -----------------------------------------------------------------------
    virtual void scanCDSection() = 0;
    virtual void scanCharData(XMLBuffer& toToUse) = 0;
    virtual EntityExpRes scanEntityRef
    (
        const   bool    inAttVal
        ,       XMLCh&  firstCh
        ,       XMLCh&  secondCh
        ,       bool&   escaped
    ) = 0;
    virtual void scanDocTypeDecl() = 0;
    virtual void scanReset(const InputSource& src) = 0;
    virtual void sendCharData(XMLBuffer& toSend) = 0;

    // -----------------------------------------------------------------------
    //  Protected scanning methods
    // -----------------------------------------------------------------------
    bool scanCharRef(XMLCh& toFill, XMLCh& second);
    void scanComment();
    bool scanEq();
    void scanMiscellaneous();
    void scanPI();
    void scanProlog();
    void scanXMLDecl(const DeclTypes type);

    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------
    void checkIDRefs();
    bool isLegalToken(const XMLPScanToken& toCheck);
    XMLTokens senseNextToken(unsigned int& orgReader);
    void initValidator(XMLValidator* theValidator);
    inline void resetValidationContext();
    unsigned int *getNewUIntPtr();
    void resetUIntPool();
    void recreateUIntPool();

    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fAttrList
    //      Every time we get a new element start tag, we have to pass to
    //      the document handler the attributes found. To make it more
    //      efficient we keep this ref vector of XMLAttr objects around. We
    //      just reuse it over and over, allowing it to grow to meet the
    //      peak need.
    //
    //  fBufMgr
    //      This is a manager for temporary buffers used during scanning.
    //      For efficiency we must use a set of static buffers, but we have
    //      to insure that they are not incorrectly reused. So this manager
    //      provides the smarts to hand out buffers as required.
    //
    //  fDocHandler
    //      The client code's document handler. If zero, then no document
    //      handler callouts are done. We don't adopt it.
    //
    //  fDocTypeHandler
    //      The client code's document type handler (used by DTD Validator).
    //
    //  fDoNamespaces
    //      This flag indicates whether the client code wants us to do
    //      namespaces or not. If the installed validator indicates that it
    //      has to do namespaces, then this is ignored.
    //
    //  fEntityHandler
    //      The client code's entity handler. If zero, then no entity handler
    //      callouts are done. We don't adopt it.
    //
    //  fErrorReporter
    //      The client code's error reporter. If zero, then no error reporter
    //      callouts are done. We don't adopt it.
    //
    //  fErrorHandler
    //      The client code's error handler.  Need to store this info for
    //      Schema parse error handling.
    //
    //  fPSVIHandler
    //      The client code's PSVI handler.
    //
    //  fExitOnFirstFatal
    //      This indicates whether we bail out on the first fatal XML error
    //      or not. It defaults to true, which is the strict XML way, but it
    //      can be changed.
    //
    //  fValidationConstraintFatal
    //      This indicates whether we treat validation constraint errors as
    //      fatal errors or not. It defaults to false, but it can be changed.
    //
    //  fIDRefList
    //      This is a list of XMLRefInfo objects. This member lets us do all
    //      needed ID-IDREF balancing checks.
    //
    //  fInException
    //      To avoid a circular freakout when we catch an exception and emit
    //      it, which would normally throw again if the 'fail on first error'
    //      flag is one.
    //
    //  fReaderMgr
    //      This is the reader manager, from which we get characters. It
    //      manages the reader stack for us, and provides a lot of convenience
    //      methods to do specialized checking for chars, sequences of chars,
    //      skipping chars, etc...
    //
    //  fScannerId
    //  fSequenceId
    //      These are used for progressive parsing, to make sure that the
    //      client code does the right thing at the right time.
    //
    //  fStandalone
    //      Indicates whether the document is standalone or not. Defaults to
    //      no, but can be overridden in the XMLDecl.
    //
    //  fHasNoDTD
    //      Indicates the document has no DTD or has only an internal DTD subset
    //      which contains no parameter entity references.
    //
    //  fValidate
    //      Indicates whether any validation should be done. This is defined
    //      by the existence of a Grammar together with fValScheme.
    //
    //  fValidator
    //      The installed validator. We look at them via the abstract
    //      validator interface, and don't know what it actual is.
    //      Either point to user's installed validator, or fDTDValidator
    //      or fSchemaValidator.
    //
    //  fValidatorFromUser
    //      This flag indicates whether the validator was installed from
    //      user.  If false, then the validator was created by the Scanner.
    //
    //  fValScheme
    //      This is the currently set validation scheme. It defaults to
    //      'never', but can be set by the client.
    //
    //  fErrorCount
    //		The number of errors we've encountered.
    //
    //  fDoSchema
    //      This flag indicates whether the client code wants Schema to
    //      be processed or not.
    //
    //  fSchemaFullChecking
    //      This flag indicates whether the client code wants full Schema
    //      constraint checking.
    //
    //  fAttName
    //  fAttValue
    //  fCDataBuf
    //  fNameBuf
    //  fQNameBuf
    //  fPrefixBuf
    //      For the most part, buffers are obtained from the fBufMgr object
    //      on the fly. However, for the start tag scan, we have a set of
    //      fixed buffers for performance reasons. These are used a lot and
    //      there are a number of them, so asking the buffer manager each
    //      time for new buffers is a bit too much overhead.
    //
    //  fEmptyNamespaceId
    //      This is the id of the empty namespace URI. This is a special one
    //      because of the xmlns="" type of deal. We have to quickly sense
    //      that its the empty namespace.
    //
    //  fUnknownNamespaceId
    //      This is the id of the namespace URI which is assigned to the
    //      global namespace. Its for debug purposes only, since there is no
    //      real global namespace URI. Its set by the derived class.
    //
    //  fXMLNamespaceId
    //  fXMLNSNamespaceId
    //      These are the ids of the namespace URIs which are assigned to the
    //      'xml' and 'xmlns' special prefixes. The former is officially
    //      defined but the latter is not, so we just provide one for debug
    //      purposes.
    //
    //  fSchemaNamespaceId
    //      This is the id of the schema namespace URI.
    //
    //  fGrammarResolver
    //      Grammar Pool that stores all the grammars. Key is namespace for
    //      schema and system id for external DTD. When caching a grammar, if
    //      a grammar is already in the pool, it will be replaced with the
    //      new parsed one.
    //
    //  fGrammar
    //      Current Grammar used by the Scanner and Validator
    //
    //  fRootGrammar
    //      The grammar where the root element is declared.
    //
    //  fGrammarType
    //      Current Grammar Type.  Store this value instead of calling getGrammarType
    //      all the time for faster performance.
    //
    //  fURIStringPool
    //      This is a pool for URIs with unique ids assigned. We use a standard
    //      string pool class.  This pool is going to be shared by all Grammar.
    //      Use only if namespace is turned on.
    //
    //  fRootElemName
    //      No matter we are using DTD or Schema Grammar, if a DOCTYPE exists,
    //      we need to verify the root element name.  So store the rootElement
    //      that is used in the DOCTYPE in the Scanner instead of in the DTDGrammar
    //      where it used to
    //
    //  fExternalSchemaLocation
    //      The list of Namespace/SchemaLocation that was specified externally
    //      using setExternalSchemaLocation.
    //
    //  fExternalNoNamespaceSchemaLocation
    //      The no target namespace XML Schema Location that was specified
    //      externally using setExternalNoNamespaceSchemaLocation.
    //
    //  fSecurityManager
    //      The SecurityManager instance; as and when set by the application.
    //
    //  fEntityExpansionLimit
    //      The number of entity expansions to be permitted while processing this document
    //      Only meaningful when fSecurityManager != 0
    //
    //  fEntityExpansionCount
    //      The number of general entities expanded so far in this document.
    //      Only meaningful when fSecurityManager != null
    //
    //  fLoadExternalDTD
    //      This flag indicates whether the external DTD be loaded or not
    //
    //  fNormalizeData
    //      This flag indicates whether the parser should perform datatype
    //      normalization that is defined in the schema.
    //
    //  fCalculateSrcOfs
    //      This flag indicates the parser should calculate the source offset.
    //      Turning this on may impact performance.
    //
    //  fStandardUriConformant
    //      This flag controls whether we force conformant URI
    //
    //  fXMLVersion
    //      Enum to indicate if the main doc is XML 1.1 or XML 1.0 conformant    
    //  fUIntPool
    //      pool of unsigned integers to help with duplicate attribute
    //      detection and filling in default/fixed attributes
    //  fUIntPoolRow
    //      current row in fUIntPool
    //  fUIntPoolCol
    //      current column i row
    //  fUIntPoolRowTotal
    //      total number of rows in table
    //
    //  fMemoryManager
    //      Pluggable memory manager for dynamic allocation/deallocation.
    // -----------------------------------------------------------------------
    bool                        fStandardUriConformant;
    bool                        fCalculateSrcOfs;
    bool                        fDoNamespaces;
    bool                        fExitOnFirstFatal;
    bool                        fValidationConstraintFatal;
    bool                        fInException;
    bool                        fStandalone;
    bool                        fHasNoDTD;
    bool                        fValidate;
    bool                        fValidatorFromUser;
    bool                        fDoSchema;
    bool                        fSchemaFullChecking;
    bool                        fToCacheGrammar;
    bool                        fUseCachedGrammar;
    bool                        fLoadExternalDTD;
    bool                        fNormalizeData;
    int                         fErrorCount;
    unsigned int                fEntityExpansionLimit;
    unsigned int                fEntityExpansionCount;
    unsigned int                fEmptyNamespaceId;
    unsigned int                fUnknownNamespaceId;
    unsigned int                fXMLNamespaceId;
    unsigned int                fXMLNSNamespaceId;
    unsigned int                fSchemaNamespaceId;
    unsigned int **             fUIntPool;
    unsigned int                fUIntPoolRow;
    unsigned int                fUIntPoolCol;
    unsigned int                fUIntPoolRowTotal;
    XMLUInt32                   fScannerId;
    XMLUInt32                   fSequenceId;
    RefVectorOf<XMLAttr>*       fAttrList;
    XMLDocumentHandler*         fDocHandler;
    DocTypeHandler*             fDocTypeHandler;
    XMLEntityHandler*           fEntityHandler;
    XMLErrorReporter*           fErrorReporter;
    ErrorHandler*               fErrorHandler;
    PSVIHandler*                fPSVIHandler;
    ValidationContext           *fValidationContext;
    bool                        fEntityDeclPoolRetrieved;
    ReaderMgr                   fReaderMgr;
    XMLValidator*               fValidator;
    ValSchemes                  fValScheme;
    GrammarResolver* const      fGrammarResolver;
    MemoryManager* const        fGrammarPoolMemoryManager;
    Grammar*                    fGrammar;
    Grammar*                    fRootGrammar;
    XMLStringPool*              fURIStringPool;
    XMLCh*                      fRootElemName;
    XMLCh*                      fExternalSchemaLocation;
    XMLCh*                      fExternalNoNamespaceSchemaLocation;
    SecurityManager*            fSecurityManager;
    XMLReader::XMLVersion       fXMLVersion;
    MemoryManager*              fMemoryManager;
    XMLBufferMgr                fBufMgr;
    XMLBuffer                   fAttNameBuf;
    XMLBuffer                   fAttValueBuf;
    XMLBuffer                   fCDataBuf;
    XMLBuffer                   fQNameBuf;
    XMLBuffer                   fPrefixBuf;
    XMLBuffer                   fURIBuf;
    ElemStack                   fElemStack;


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLScanner();
    XMLScanner(const XMLScanner&);
    XMLScanner& operator=(const XMLScanner&);

    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------
    void commonInit();

    // -----------------------------------------------------------------------
    //  Private scanning methods
    // -----------------------------------------------------------------------
    bool getQuotedString(XMLBuffer& toFill);
    unsigned int scanUpToWSOr
    (
                XMLBuffer&  toFill
        , const XMLCh       chEndChar
    );
};

// ---------------------------------------------------------------------------
//  XMLScanner: Getter methods
// ---------------------------------------------------------------------------
inline const XMLDocumentHandler* XMLScanner::getDocHandler() const
{
    return fDocHandler;
}

inline XMLDocumentHandler* XMLScanner::getDocHandler()
{
    return fDocHandler;
}

inline const DocTypeHandler* XMLScanner::getDocTypeHandler() const
{
    return fDocTypeHandler;
}

inline DocTypeHandler* XMLScanner::getDocTypeHandler()
{
    return fDocTypeHandler;
}

inline bool XMLScanner::getDoNamespaces() const
{
    return fDoNamespaces;
}

inline const XMLEntityHandler* XMLScanner::getEntityHandler() const
{
    return fEntityHandler;
}

inline XMLEntityHandler* XMLScanner::getEntityHandler()
{
    return fEntityHandler;
}

inline const XMLErrorReporter* XMLScanner::getErrorReporter() const
{
    return fErrorReporter;
}

inline XMLErrorReporter* XMLScanner::getErrorReporter()
{
    return fErrorReporter;
}

inline const ErrorHandler* XMLScanner::getErrorHandler() const
{
    return fErrorHandler;
}

inline ErrorHandler* XMLScanner::getErrorHandler()
{
    return fErrorHandler;
}

inline const PSVIHandler* XMLScanner::getPSVIHandler() const
{
    return fPSVIHandler;
}

inline PSVIHandler* XMLScanner::getPSVIHandler()
{
    return fPSVIHandler;
}

inline bool XMLScanner::getExitOnFirstFatal() const
{
    return fExitOnFirstFatal;
}

inline bool XMLScanner::getValidationConstraintFatal() const
{
    return fValidationConstraintFatal;
}

inline bool XMLScanner::getInException() const
{
    return fInException;
}

inline RefHashTableOf<XMLRefInfo>* XMLScanner::getIDRefList()
{
    return fValidationContext->getIdRefList();
}

inline const RefHashTableOf<XMLRefInfo>* XMLScanner::getIDRefList() const
{
    return fValidationContext->getIdRefList();
}

inline ValidationContext*  XMLScanner::getValidationContext()
{
    if (!fEntityDeclPoolRetrieved)
    {
        fValidationContext->setEntityDeclPool(getEntityDeclPool());
        fEntityDeclPoolRetrieved = true;
    }

    return fValidationContext;
}

inline const Locator* XMLScanner::getLocator() const
{
    return &fReaderMgr;
}

inline const ReaderMgr* XMLScanner::getReaderMgr() const
{
    return &fReaderMgr;
}

inline unsigned int XMLScanner::getSrcOffset() const
{
    return fReaderMgr.getSrcOffset();
}

inline bool XMLScanner::getStandalone() const
{
    return fStandalone;
}

inline XMLScanner::ValSchemes XMLScanner::getValidationScheme() const
{
    return fValScheme;
}

inline const XMLValidator* XMLScanner::getValidator() const
{
    return fValidator;
}

inline XMLValidator* XMLScanner::getValidator()
{
    return fValidator;
}

inline bool XMLScanner::getDoSchema() const
{
    return fDoSchema;
}

inline bool XMLScanner::getValidationSchemaFullChecking() const
{
    return fSchemaFullChecking;
}

inline int XMLScanner::getErrorCount()
{
    return fErrorCount;
}

inline bool XMLScanner::isValidatorFromUser()
{
    return fValidatorFromUser;
}

inline unsigned int XMLScanner::getEmptyNamespaceId() const
{
    return fEmptyNamespaceId;
}

inline unsigned int XMLScanner::getUnknownNamespaceId() const
{
    return fUnknownNamespaceId;
}

inline unsigned int XMLScanner::getXMLNamespaceId() const
{
    return fXMLNamespaceId;
}

inline unsigned int XMLScanner::getXMLNSNamespaceId() const
{
    return fXMLNSNamespaceId;
}

inline const XMLStringPool* XMLScanner::getURIStringPool() const
{
    return fURIStringPool;
}

inline XMLStringPool* XMLScanner::getURIStringPool()
{
    return fURIStringPool;
}

inline bool XMLScanner::getHasNoDTD() const
{
    return fHasNoDTD;
}

inline XMLCh* XMLScanner::getExternalSchemaLocation() const
{
    return fExternalSchemaLocation;
}

inline XMLCh* XMLScanner::getExternalNoNamespaceSchemaLocation() const
{
    return fExternalNoNamespaceSchemaLocation;
}

inline SecurityManager* XMLScanner::getSecurityManager() const
{
    return fSecurityManager;
}

inline bool XMLScanner::getLoadExternalDTD() const
{
    return fLoadExternalDTD;
}

inline bool XMLScanner::getNormalizeData() const
{
    return fNormalizeData;
}

inline bool XMLScanner::isCachingGrammarFromParse() const
{
    return fToCacheGrammar;
}

inline bool XMLScanner::isUsingCachedGrammarInParse() const
{
    return fUseCachedGrammar;
}

inline bool XMLScanner::getCalculateSrcOfs() const
{
    return fCalculateSrcOfs;
}

inline Grammar* XMLScanner::getRootGrammar() const
{
    return fRootGrammar;
}

inline bool XMLScanner::getStandardUriConformant() const
{
    return fStandardUriConformant;
}

inline XMLReader::XMLVersion XMLScanner::getXMLVersion() const
{
	return fXMLVersion;
}

inline MemoryManager* XMLScanner::getMemoryManager() const
{
    return fMemoryManager;
}

inline ValueVectorOf<PrefMapElem*>* XMLScanner::getNamespaceContext() const
{
    return fElemStack.getNamespaceMap();
}

inline unsigned int XMLScanner::getPrefixId(const XMLCh* const prefix) const
{
    return fElemStack.getPrefixId(prefix);
}

inline const XMLCh* XMLScanner::getPrefixForId(unsigned int prefId) const
{
    return fElemStack.getPrefixForId(prefId);
}

// ---------------------------------------------------------------------------
//  XMLScanner: Setter methods
// ---------------------------------------------------------------------------
inline void XMLScanner::setDocHandler(XMLDocumentHandler* const docHandler)
{
    fDocHandler = docHandler;
}

inline void XMLScanner::setDocTypeHandler(DocTypeHandler* const docTypeHandler)
{
    fDocTypeHandler = docTypeHandler;
}

inline void XMLScanner::setErrorHandler(ErrorHandler* const handler)
{
    fErrorHandler = handler;
}

inline void XMLScanner::setPSVIHandler(PSVIHandler* const handler)
{
    fPSVIHandler = handler;
}

inline void XMLScanner::setEntityHandler(XMLEntityHandler* const entityHandler)
{
    fEntityHandler = entityHandler;
    fReaderMgr.setEntityHandler(entityHandler);
}

inline void XMLScanner::setErrorReporter(XMLErrorReporter* const errHandler)
{
    fErrorReporter = errHandler;
}

inline void XMLScanner::setExitOnFirstFatal(const bool newValue)
{
    fExitOnFirstFatal = newValue;
}


inline void XMLScanner::setValidationConstraintFatal(const bool newValue)
{
    fValidationConstraintFatal = newValue;
}

inline void XMLScanner::setValidationScheme(const ValSchemes newScheme)
{
    fValScheme = newScheme;

    // validation flag for Val_Auto is set to false by default,
    //   and will be turned to true if a grammar is seen
    if (fValScheme == Val_Always)
        fValidate = true;
    else
        fValidate = false;
}

inline void XMLScanner::setDoSchema(const bool doSchema)
{
    fDoSchema = doSchema;
}

inline void XMLScanner::setDoNamespaces(const bool doNamespaces)
{
    fDoNamespaces = doNamespaces;
}

inline void XMLScanner::setValidationSchemaFullChecking(const bool schemaFullChecking)
{
    fSchemaFullChecking = schemaFullChecking;
}

inline void XMLScanner::setHasNoDTD(const bool hasNoDTD)
{
    fHasNoDTD = hasNoDTD;
}

inline void XMLScanner::setRootElemName(XMLCh* rootElemName)
{
    fMemoryManager->deallocate(fRootElemName);//delete [] fRootElemName;
    fRootElemName = XMLString::replicate(rootElemName, fMemoryManager);
}

inline void XMLScanner::setExternalSchemaLocation(const XMLCh* const schemaLocation)
{
    fMemoryManager->deallocate(fExternalSchemaLocation);//delete [] fExternalSchemaLocation;
    fExternalSchemaLocation = XMLString::replicate(schemaLocation, fMemoryManager);
}

inline void XMLScanner::setExternalNoNamespaceSchemaLocation(const XMLCh* const noNamespaceSchemaLocation)
{
    fMemoryManager->deallocate(fExternalNoNamespaceSchemaLocation);//delete [] fExternalNoNamespaceSchemaLocation;
    fExternalNoNamespaceSchemaLocation = XMLString::replicate(noNamespaceSchemaLocation, fMemoryManager);
}

inline void XMLScanner::setExternalSchemaLocation(const char* const schemaLocation)
{
    fMemoryManager->deallocate(fExternalSchemaLocation);//delete [] fExternalSchemaLocation;
    fExternalSchemaLocation = XMLString::transcode(schemaLocation, fMemoryManager);
}

inline void XMLScanner::setExternalNoNamespaceSchemaLocation(const char* const noNamespaceSchemaLocation)
{
    fMemoryManager->deallocate(fExternalNoNamespaceSchemaLocation);//delete [] fExternalNoNamespaceSchemaLocation;
    fExternalNoNamespaceSchemaLocation = XMLString::transcode(noNamespaceSchemaLocation, fMemoryManager);
}

inline void XMLScanner::setSecurityManager(SecurityManager* const securityManager)
{
    fSecurityManager = securityManager;
    if(securityManager != 0) 
    {
        fEntityExpansionLimit = securityManager->getEntityExpansionLimit();
        fEntityExpansionCount = 0;
    }
}

inline void XMLScanner::setLoadExternalDTD(const bool loadDTD)
{
    fLoadExternalDTD = loadDTD;
}

inline void XMLScanner::setNormalizeData(const bool normalizeData)
{
    fNormalizeData = normalizeData;
}

inline void XMLScanner::cacheGrammarFromParse(const bool newValue)
{
    fToCacheGrammar = newValue;
}

inline void XMLScanner::useCachedGrammarInParse(const bool newValue)
{
    fUseCachedGrammar = newValue;
}

inline void XMLScanner::setCalculateSrcOfs(const bool newValue)
{
    fCalculateSrcOfs = newValue;
}

inline void XMLScanner::setStandardUriConformant(const bool newValue)
{
    fStandardUriConformant = newValue;
    fReaderMgr.setStandardUriConformant(newValue);
}

// ---------------------------------------------------------------------------
//  XMLScanner: Mutator methods
// ---------------------------------------------------------------------------
inline void XMLScanner::incrementErrorCount()
{
    ++fErrorCount;
}

// ---------------------------------------------------------------------------
//  XMLScanner: Deprecated methods
// ---------------------------------------------------------------------------
inline bool XMLScanner::getDoValidation() const
{
    return fValidate;
}

inline void XMLScanner::setDoValidation(const bool validate)
{
    fValidate = validate;
    if (fValidate)
        fValScheme = Val_Always;
    else
        fValScheme = Val_Never;
}

inline void XMLScanner::resetValidationContext()
{
    fValidationContext->clearIdRefList();
    fValidationContext->setEntityDeclPool(0);
    fEntityDeclPoolRetrieved = false;
}

XERCES_CPP_NAMESPACE_END

#endif

