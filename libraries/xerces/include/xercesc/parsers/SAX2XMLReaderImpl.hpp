/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2003 The Apache Software Foundation.  All rights
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
 * $Log: SAX2XMLReaderImpl.hpp,v $
 * Revision 1.24  2003/11/06 15:30:07  neilg
 * first part of PSVI/schema component model implementation, thanks to David Cargill.  This covers setting the PSVIHandler on parser objects, as well as implementing XSNotation, XSSimpleTypeDefinition, XSIDCDefinition, and most of XSWildcard, XSComplexTypeDefinition, XSElementDeclaration, XSAttributeDeclaration and XSAttributeUse.
 *
 * Revision 1.23  2003/10/30 21:37:31  knoaman
 * Enhanced Entity Resolver Support. Thanks to David Cargill.
 *
 * Revision 1.22  2003/06/20 18:55:54  peiyongz
 * Stateless Grammar Pool :: Part I
 *
 * Revision 1.21  2003/05/22 02:10:51  knoaman
 * Default the memory manager.
 *
 * Revision 1.20  2003/05/15 18:26:50  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.19  2003/04/17 21:58:50  neilg
 * Adding a new property,
 * http://apache.org/xml/properties/security-manager, with
 * appropriate getSecurityManager/setSecurityManager methods on DOM
 * and SAX parsers.  Also adding a new SecurityManager class.
 *
 * The purpose of these modifications is to permit applications a
 * means to have the parser reject documents whose processing would
 * otherwise consume large amounts of system resources.  Malicious
 * use of such documents could be used to launch a denial-of-service
 * attack against a system running the parser.  Initially, the
 * SecurityManager only knows about attacks that can result from
 * exponential entity expansion; this is the only known attack that
 * involves processing a single XML document.  Other, simlar attacks
 * can be launched if arbitrary schemas may be parsed; there already
 * exist means (via use of the EntityResolver interface) by which
 * applications can deny processing of untrusted schemas.  In future,
 * the SecurityManager will be expanded to take these other exploits
 * into account.
 *
 * Adding SecurityManager support
 * 
 * Revision 1.18  2003/03/07 18:09:16  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.17  2003/01/09 19:07:08  tng
 * [Bug 15802] Add "const" qualifier to getURIText.
 *
 * Revision 1.16  2002/12/23 15:23:18  knoaman
 * Added a public api to various parsers to return the src offset within the input
 * source.
 *
 * Revision 1.15  2002/12/04 01:57:09  knoaman
 * Scanner re-organization.
 *
 * Revision 1.14  2002/11/04 14:57:03  tng
 * C++ Namespace Support.
 *
 * Revision 1.13  2002/08/14 15:20:38  knoaman
 * [Bug 3111] Problem with LexicalHandler::startDTD() and LexicalHandler::endDTD().
 *
 * Revision 1.12  2002/07/11 18:27:04  knoaman
 * Grammar caching/preparsing - initial implementation.
 *
 * Revision 1.11  2002/06/27 18:47:32  tng
 * API Documentation Update.
 *
 * Revision 1.10  2002/06/17 15:41:15  tng
 * To be consistent, SAX2 is updated with:
 * 1. the progressive parse methods should use the fReuseGrammar flag set from setFeature instead of using parameter
 * 2. add feature "http://apache.org/xml/features/continue-after-fatal-error", and users should use setFeature instead of setExitOnFirstFatalError
 * 3. add feature "http://apache.org/xml/features/validation-error-as-fatal", and users should use setFeature instead of setValidationConstraintFatal
 *
 * Revision 1.9  2002/06/06 20:38:18  tng
 * Document Fix: document that the returned object from resolveEntity is owned by the parser
 *
 * Revision 1.8  2002/05/31 15:13:53  tng
 * Fix doxygen documentation.
 *
 * Revision 1.7  2002/05/30 16:39:06  knoaman
 * DOM L3 LS.
 *
 * Revision 1.6  2002/05/30 16:20:09  tng
 * Add feature to optionally ignore external DTD.
 *
 * Revision 1.5  2002/05/29 21:37:47  knoaman
 * Add baseURI to resolveEntity to support DOMInputSource.
 *
 * Revision 1.4  2002/05/28 20:44:14  tng
 * [Bug 9104] prefixes dissapearing when schema validation turned on.
 *
 * Revision 1.3  2002/05/27 18:39:21  tng
 * To get ready for 64 bit large file, use XMLSSize_t to represent line and column number.
 *
 * Revision 1.2  2002/02/13 16:09:24  knoaman
 * Move SAX2 features/properties names constants to XMLUni.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:07  peiyongz
 * sane_include
 *
 * Revision 1.21  2002/01/28 18:45:40  knoaman
 * Update documentation for SAX2 feature 'namespace-prefixes'.
 *
 * Revision 1.20  2002/01/28 17:08:47  knoaman
 * SAX2-ext's DeclHandler support.
 *
 * Revision 1.19  2002/01/24 16:30:34  tng
 * [Bug 3111] Problem with LexicalHandler::startDTD() and LexicalHandler::endDTD() .
 *
 * Revision 1.18  2002/01/18 16:31:38  tng
 * Break program.xml which takes too long to load, into program-sax.xml, program-sax2.xml, program-dom.xml, program-idom.xml.
 *
 * Revision 1.17  2002/01/02 15:36:41  tng
 * Some documentation update.
 *
 * Revision 1.16  2001/11/20 18:51:44  tng
 * Schema: schemaLocation and noNamespaceSchemaLocation to be specified outside the instance document.  New methods setExternalSchemaLocation and setExternalNoNamespaceSchemaLocation are added (for SAX2, two new properties are added).
 *
 * Revision 1.15  2001/11/14 14:15:42  tng
 * Update SAX2 feature documentation.
 *
 * Revision 1.14  2001/09/12 13:03:43  tng
 * [Bug 3155] SAX2 does not offer progressive parse.
 *
 * Revision 1.13  2001/08/01 19:11:02  tng
 * Add full schema constraint checking flag to the samples and the parser.
 *
 * Revision 1.12  2001/06/27 17:39:52  knoaman
 * Fix for bug #2353.
 *
 * Revision 1.11  2001/06/19 16:45:08  tng
 * Add installAdvDocHandler to SAX2XMLReader as the code is there already.
 *
 * Revision 1.10  2001/06/04 21:01:49  jberry
 * getErrorCount is virtual in this class reflecting derivation from SAX2XMLReader.
 *
 * Revision 1.9  2001/06/03 19:26:19  jberry
 * Add support for querying error count following parse; enables simple parse without requiring error handler.
 *
 * Revision 1.8  2001/05/11 13:26:21  tng
 * Copyright update.
 *
 * Revision 1.7  2001/03/30 16:46:57  tng
 * Schema: Use setDoSchema instead of setSchemaValidation which makes more sense.
 *
 * Revision 1.6  2001/03/21 21:56:08  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
 * Revision 1.5  2001/02/15 15:56:29  tng
 * Schema: Add setSchemaValidation and getSchemaValidation for DOMParser and SAXParser.
 * Add feature "http://apache.org/xml/features/validation/schema" for SAX2XMLReader.
 * New data field  fSchemaValidation in XMLScanner as the flag.
 *
 * Revision 1.4  2000/12/22 15:16:53  tng
 * SAX2-ext's LexicalHandler support added by David Bertoni.
 *
 * Revision 1.3  2000/08/09 22:16:13  jpolast
 * many conformance & stability changes:
 *   - ContentHandler::resetDocument() removed
 *   - attrs param of ContentHandler::startDocument() made const
 *   - SAXExceptions thrown now have msgs
 *   - removed duplicate function signatures that had 'const'
 *       [ eg: getContentHander() ]
 *   - changed getFeature and getProperty to apply to const objs
 *   - setProperty now takes a void* instead of const void*
 *   - SAX2XMLReaderImpl does not inherit from SAXParser anymore
 *   - Reuse Validator (http://apache.org/xml/features/reuse-validator) implemented
 *   - Features & Properties now read-only during parse
 *
 * Revision 1.2  2000/08/02 20:46:32  aruna1
 * sax2 changes
 *
 * Revision 1.1  2000/08/02 18:04:41  jpolast
 * initial checkin of sax2 implemenation
 * submitted by Simon Fell (simon@fell.com)
 * and Joe Polastre (jpolast@apache.org)
 *
 *
 */

#if !defined(SAX2XMLReaderImpl_HPP)
#define SAX2XMLReaderImpl_HPP

#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/sax/Parser.hpp>
#include <xercesc/framework/XMLBuffer.hpp>
#include <xercesc/internal/VecAttributesImpl.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/util/RefStackOf.hpp>
#include <xercesc/util/SecurityManager.hpp>
#include <xercesc/util/ValueStackOf.hpp>
#include <xercesc/framework/XMLBufferMgr.hpp>

XERCES_CPP_NAMESPACE_BEGIN


class ContentHandler;
class LexicalHandler;
class DeclHandler;
class GrammarResolver;
class XMLGrammarPool;
class XMLResourceIdentifier;
class PSVIHandler;

/**
  * This class implements the SAX2 'XMLReader' interface and should be
  * used by applications wishing to parse the XML files using SAX2.
  * It allows the client program to install SAX2 handlers for event
  * callbacks.
  *
  * <p>It can be used to instantiate a validating or non-validating
  * parser, by setting a member flag.</p>
  *
  * we basically re-use the existing SAX1 parser code, but provide a
  * new implementation of XMLContentHandler that raises the new
  * SAX2 style events
  *
  */

class PARSERS_EXPORT SAX2XMLReaderImpl :
	public XMemory
    , public SAX2XMLReader
    , public XMLDocumentHandler
    , public XMLErrorReporter
    , public XMLEntityHandler
    , public DocTypeHandler
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructors and Destructor */
    //@{
    /** The default constructor */
	SAX2XMLReaderImpl(
                            MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
                          , XMLGrammarPool* const gramPool = 0 
                          );

    /** The destructor */	
	~SAX2XMLReaderImpl() ;
   //@}

    //-----------------------------------------------------------------------
    // Implementation of SAX2XMLReader Interface
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // The XMLReader interface
    //-----------------------------------------------------------------------
    /** @name Implementation of SAX 2.0 XMLReader interface's. */
    //@{

    /**
      * This method returns the installed content handler.
      *
      * @return A pointer to the installed content handler object.
      */
    virtual ContentHandler* getContentHandler() const ;

    /**
      * This method returns the installed DTD handler.
      *
      * @return A pointer to the installed DTD handler object.
      */
    virtual DTDHandler* getDTDHandler() const ;

    /**
      * This method returns the installed entity resolver.
      *
      * @return A pointer to the installed entity resolver object.
      */
    virtual EntityResolver* getEntityResolver() const ;

    /**
      * This method returns the installed entity resolver.
      *
      * @return A pointer to the installed entity resolver object.
      */
    virtual XMLEntityResolver* getXMLEntityResolver() const ;

    /**
      * This method returns the installed error handler.
      *
      * @return A pointer to the installed error handler object.
      */
    virtual ErrorHandler* getErrorHandler() const ;

    /**
      * This method returns the installed PSVI handler.
      *
      * @return A pointer to the installed PSVI handler object.
      */
    virtual PSVIHandler* getPSVIHandler() const ;

	/**
     * Query the current state of any feature in a SAX2 XMLReader.
	  *
	  * @param name The unique identifier (URI) of the feature being set.
	  * @return The current state of the feature.
     * @exception SAXNotRecognizedException If the requested feature is not known.
	  */
	virtual bool getFeature(const XMLCh* const name) const ;

	/**
     * Query the current value of a property in a SAX2 XMLReader.
     *
     * The parser owns the returned pointer.  The memory allocated for
     * the returned pointer will be destroyed when the parser is deleted.
     *
     * To ensure assessiblity of the returned information after the parser
     * is deleted, callers need to copy and store the returned information
     * somewhere else; otherwise you may get unexpected result.  Since the returned
     * pointer is a generic void pointer, see
     * http://xml.apache.org/xerces-c/program-sax2.html#SAX2Properties to learn
     * exactly what type of property value each property returns for replication.
     *
     * @param name The unique identifier (URI) of the property being set.
     * @return     The current value of the property.  The pointer spans the same
     *             life-time as the parser.  A null pointer is returned if nothing
     *             was specified externally.
     * @exception  SAXNotRecognizedException If the requested property is not known.
     */
	virtual void* getProperty(const XMLCh* const name) const ;

  /**
    * Allow an application to register a document event handler.
    *
    * If the application does not register a document handler, all
    * document events reported by the SAX parser will be silently
    * ignored (this is the default behaviour implemented by
    * HandlerBase).
    *
    * Applications may register a new or different handler in the
    * middle of a parse, and the SAX parser must begin using the new
    * handler immediately.
    *
    * @param handler The document handler.
    * @see DocumentHandler#DocumentHandler
    * @see HandlerBase#HandlerBase
    */
    virtual void setContentHandler(ContentHandler* const handler) ;

  /**
    * Allow an application to register a DTD event handler.
    *
    * If the application does not register a DTD handler, all DTD
    * events reported by the SAX parser will be silently ignored (this
    * is the default behaviour implemented by HandlerBase).
    *
    * Applications may register a new or different handler in the middle
    * of a parse, and the SAX parser must begin using the new handler
    * immediately.
    *
    * @param handler The DTD handler.
    * @see DTDHandler#DTDHandler
    * @see HandlerBase#HandlerBase
    */
    virtual void setDTDHandler(DTDHandler* const handler) ;

  /**
    * Allow an application to register a custom entity resolver.
    *
    * If the application does not register an entity resolver, the
    * SAX parser will resolve system identifiers and open connections
    * to entities itself (this is the default behaviour implemented in
    * DefaultHandler).
    *
    * Applications may register a new or different entity resolver
    * in the middle of a parse, and the SAX parser must begin using
    * the new resolver immediately.
    *
    * <i>Any previously set entity resolver is merely dropped, since the parser
    * does not own them.  If both setEntityResolver and setXMLEntityResolver
    * are called, then the last one is used.</i>
    *
    * @param resolver The object for resolving entities.
    * @see EntityResolver#EntityResolver
    * @see DefaultHandler#DefaultHandler
    */
    virtual void setEntityResolver(EntityResolver* const resolver) ;
    
  /** Set the entity resolver
    *
    * This method allows applications to install their own entity
    * resolver. By installing an entity resolver, the applications
    * can trap and potentially redirect references to external
    * entities.
    *
    * <i>Any previously set entity resolver is merely dropped, since the parser
    * does not own them.  If both setEntityResolver and setXMLEntityResolver
    * are called, then the last one is used.</i>
    *
    * @param resolver  A const pointer to the user supplied entity
    *                  resolver.
    *
    * @see #getXMLEntityResolver
    */
    virtual void setXMLEntityResolver(XMLEntityResolver* const resolver) ;

  /**
    * Allow an application to register an error event handler.
    *
    * If the application does not register an error event handler,
    * all error events reported by the SAX parser will be silently
    * ignored, except for fatalError, which will throw a SAXException
    * (this is the default behaviour implemented by HandlerBase).
    *
    * Applications may register a new or different handler in the
    * middle of a parse, and the SAX parser must begin using the new
    * handler immediately.
    *
    * @param handler The error handler.
    * @see ErrorHandler#ErrorHandler
    * @see SAXException#SAXException
    * @see HandlerBase#HandlerBase
    */
    virtual void setErrorHandler(ErrorHandler* const handler) ;

  /**
    * This method installs the user specified PSVI handler on
    * the parser.
    *
    * @param handler A pointer to the PSVI handler to be called
    *                when the parser comes across 'PSVI' events
    *                as per the schema specification.
    */
    virtual void setPSVIHandler(PSVIHandler* const handler);

  /**
    * Set the state of any feature in a SAX2 XMLReader.
    * Supported features in SAX2 for xerces-c are:
    * <br>(See http://xml.apache.org/xerces-c/program-sax2.html#SAX2Features for detail description).
    *
    * <br>http://xml.org/sax/features/validation (default: true)
    * <br>http://xml.org/sax/features/namespaces (default: true)
    * <br>http://xml.org/sax/features/namespace-prefixes (default: false)
    * <br>http://apache.org/xml/features/validation/dynamic (default: false)
    * <br>http://apache.org/xml/features/validation/reuse-grammar (default: false)
    * <br>http://apache.org/xml/features/validation/schema (default: true)
    * <br>http://apache.org/xml/features/validation/schema-full-checking (default: false)
    * <br>http://apache.org/xml/features/nonvalidating/load-external-dtd (default: true)
    * <br>http://apache.org/xml/features/continue-after-fatal-error (default: false)
    * <br>http://apache.org/xml/features/validation-error-as-fatal (default: false)
    * <br>http://apache.org/xml/features/validation/reuse-validator (Deprecated) (default: false)
    *
    * @param name The unique identifier (URI) of the feature.
    * @param value The requested state of the feature (true or false).
    * @exception SAXNotRecognizedException If the requested feature is not known.
    * @exception SAXNotSupportedException Feature modification is not supported during parse
    *
    */
	virtual void setFeature(const XMLCh* const name, const bool value) ;

  /**
    * Set the value of any property in a SAX2 XMLReader.
    * Supported properties in SAX2 for xerces-c are:
    * <br>(See http://xml.apache.org/xerces-c/program-sax2.html#SAX2Properties for detail description).
    *
    * <br>http://apache.org/xml/properties/schema/external-schemaLocation
    * <br>http://apache.org/xml/properties/schema/external-noNamespaceSchemaLocation.
    *
    * It takes a void pointer as the property value.  Application is required to initialize this void
    * pointer to a correct type.  See http://xml.apache.org/xerces-c/program-sax2.html#SAX2Properties
    * to learn exactly what type of property value each property expects for processing.
    * Passing a void pointer that was initialized with a wrong type will lead to unexpected result.
    * If the same property is set more than once, the last one takes effect.
    *
    * @param name The unique identifier (URI) of the property being set.
    * @param value The requested value for the property.  See
    *            http://xml.apache.org/xerces-c/program-sax2.html#SAX2Properties to learn
    *            exactly what type of property value each property expects for processing.
    *            Passing a void pointer that was initialized with a wrong type will lead
    *            to unexpected result.
    * @exception SAXNotRecognizedException If the requested property is not known.
    * @exception SAXNotSupportedException Property modification is not supported during parse
    */
	virtual void setProperty(const XMLCh* const name, void* value) ;

  /**
    * Parse an XML document.
    *
    * The application can use this method to instruct the SAX parser
    * to begin parsing an XML document from any valid input
    * source (a character stream, a byte stream, or a URI).
    *
    * Applications may not invoke this method while a parse is in
    * progress (they should create a new Parser instead for each
    * additional XML document).  Once a parse is complete, an
    * application may reuse the same Parser object, possibly with a
    * different input source.
    *
    * @param source The input source for the top-level of the
    *               XML document.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @exception XMLException An exception from the parser or client
    *            handler code.
    * @see InputSource#InputSource
    * @see #setEntityResolver
    * @see #setDTDHandler
    * @see #setDocumentHandler
    * @see #setErrorHandler
    */
    virtual void parse
    (
        const   InputSource&    source
    ) ;

  /**
    * Parse an XML document from a system identifier (URI).
    *
    * This method is a shortcut for the common case of reading a
    * document from a system identifier.  It is the exact equivalent
    * of the following:
    *
    * parse(new URLInputSource(systemId));
    *
    * If the system identifier is a URL, it must be fully resolved
    * by the application before it is passed to the parser.
    *
    * @param systemId The system identifier (URI).
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @exception XMLException An exception from the parser or client
    *            handler code.
    * @see #parse(InputSource)
    */
    virtual void parse
    (
        const   XMLCh* const    systemId
    ) ;

  /**
    * Parse an XML document from a system identifier (URI).
    *
    * This method is a shortcut for the common case of reading a
    * document from a system identifier.  It is the exact equivalent
    * of the following:
    *
    * parse(new URLInputSource(systemId));
    *
    * If the system identifier is a URL, it must be fully resolved
    * by the application before it is passed to the parser.
    *
    * @param systemId The system identifier (URI).
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @exception XMLException An exception from the parser or client
    *            handler code.
    * @see #parse(InputSource)
    */
    virtual void parse
    (
        const   char* const     systemId
    ) ;
	
    //@}

    // -----------------------------------------------------------------------
    //  SAX 2.0-ext
    // -----------------------------------------------------------------------
    /** @name SAX 2.0-ext */
    //@{
    /**
      * This method returns the installed declaration handler.
      *
      * @return A pointer to the installed declaration handler object.
      */
    virtual DeclHandler* getDeclarationHandler() const ;

	/**
      * This method returns the installed lexical handler.
      *
      * @return A pointer to the installed lexical handler object.
      */
    virtual LexicalHandler* getLexicalHandler() const ;

   /**
    * Allow an application to register a declaration event handler.
    *
    * If the application does not register a declaration handler,
    * all events reported by the SAX parser will be silently
    * ignored. (this is the default behaviour implemented by DefaultHandler).
    *
    * Applications may register a new or different handler in the
    * middle of a parse, and the SAX parser must begin using the new
    * handler immediately.
    *
    * @param handler The DTD declaration handler.
    * @see DeclHandler#DeclHandler
    * @see SAXException#SAXException
    * @see DefaultHandler#DefaultHandler
    */
    virtual void setDeclarationHandler(DeclHandler* const handler) ;

   /**
    * Allow an application to register a lexical event handler.
    *
    * If the application does not register a lexical handler,
    * all events reported by the SAX parser will be silently
    * ignored. (this is the default behaviour implemented by HandlerBase).
    *
    * Applications may register a new or different handler in the
    * middle of a parse, and the SAX parser must begin using the new
    * handler immediately.
    *
    * @param handler The error handler.
    * @see LexicalHandler#LexicalHandler
    * @see SAXException#SAXException
    * @see HandlerBase#HandlerBase
    */
    virtual void setLexicalHandler(LexicalHandler* const handler) ;

    //@}

    // -----------------------------------------------------------------------
    //  Getter Methods
    // -----------------------------------------------------------------------
    /** @name Getter Methods (Xerces-C specific) */
    //@{
    /**
	  * This method is used to get the current validator.
	  *
	  * <b>SAX2XMLReader assumes responsibility for the validator.  It will be
	  * deleted when the XMLReader is destroyed.</b>
	  *
	  * @return A pointer to the validator.  An application should not deleted
	  * the object returned.
	  *
	  */
	virtual XMLValidator* getValidator() const ;
    //@}

    /** Get error count from the last parse operation.
      *
      * This method returns the error count from the last parse
      * operation. Note that this count is actually stored in the
      * scanner, so this method simply returns what the
      * scanner reports.
      *
      * @return number of errors encountered during the latest
      *			parse operation.
      */
    virtual int getErrorCount() const ;

    /**
      * This method returns the state of the parser's
      * exit-on-First-Fatal-Error flag.
      *
      * <p>Or you can query the feature "http://apache.org/xml/features/continue-after-fatal-error"
      * which indicates the opposite state.</p>
      *
      * @return true, if the parser is currently configured to
      *         exit on the first fatal error, false otherwise.
      *
      * @see #setExitOnFirstFatalError
      * @see #getFeature
      */
    virtual bool getExitOnFirstFatalError() const ;

    /**
      * This method returns the state of the parser's
      * validation-constraint-fatal flag.
      *
      * <p>Or you can query the feature "http://apache.org/xml/features/validation-error-as-fatal"
      * which means the same thing.
      *
      * @return true, if the parser is currently configured to
      *         set validation constraint errors as fatal, false
      *         otherwise.
      *
      * @see #setValidationContraintFatal
      * @see #getFeature
      */
    virtual bool getValidationConstraintFatal() const ;

    /**
      * Retrieve the grammar that is associated with the specified namespace key
      *
      * @param  nameSpaceKey Namespace key
      * @return Grammar associated with the Namespace key.
      */
    virtual Grammar* getGrammar(const XMLCh* const nameSpaceKey);

    /**
      * Retrieve the grammar where the root element is declared.
      *
      * @return Grammar where root element declared
      */
    virtual Grammar* getRootGrammar();

    /**
      * Returns the string corresponding to a URI id from the URI string pool.
      *
      * @param uriId id of the string in the URI string pool.
      * @return URI string corresponding to the URI id.
      */
    virtual const XMLCh* getURIText(unsigned int uriId) const;

    /**
      * Returns the current src offset within the input source.
      *
      * @return offset within the input source
      */
    virtual unsigned int getSrcOffset() const;

    //@}

    // -----------------------------------------------------------------------
    //  Setter Methods
    // -----------------------------------------------------------------------
    /** @name Setter Methods (Xerces-C specific) */
    //@{
    /**
	  * This method is used to set a validator.
	  *
	  * <b>SAX2XMLReader assumes responsibility for the validator.  It will be
	  * deleted when the XMLReader is destroyed.</b>
	  *
	  * @param valueToAdopt A pointer to the validator that the reader should use.
	  *
	  */
	virtual void setValidator(XMLValidator* valueToAdopt) ;

    /**
      * This method allows users to set the parser's behaviour when it
      * encounters the first fatal error. If set to true, the parser
      * will exit at the first fatal error. If false, then it will
      * report the error and continue processing.
      *
      * <p>The default value is 'true' and the parser exits on the
      * first fatal error.</p>
      *
      * <p>Or you can set the feature "http://apache.org/xml/features/continue-after-fatal-error"
      * which has the opposite behaviour.</p>
      *
      * <p>If both the feature above and this function are used, the latter takes effect.</p>
      *
      * @param newState The value specifying whether the parser should
      *                 continue or exit when it encounters the first
      *                 fatal error.
      *
      * @see #getExitOnFirstFatalError
      * @see #setFeature
      */
    virtual void setExitOnFirstFatalError(const bool newState) ;

    /**
      * This method allows users to set the parser's behaviour when it
      * encounters a validtion constraint error. If set to true, and the
      * the parser will treat validation error as fatal and will exit depends on the
      * state of "getExitOnFirstFatalError". If false, then it will
      * report the error and continue processing.
      *
      * Note: setting this true does not mean the validation error will be printed with
      * the word "Fatal Error".   It is still printed as "Error", but the parser
      * will exit if "setExitOnFirstFatalError" is set to true.
      *
      * <p>The default value is 'false'.</p>
      *
      * <p>Or you can set the feature "http://apache.org/xml/features/validation-error-as-fatal"
      * which means the same thing.</p>
      *
      * <p>If both the feature above and this function are used, the latter takes effect.</p>
      *
      * @param newState If true, the parser will exit if "setExitOnFirstFatalError"
      *                 is set to true.
      *
      * @see #getValidationConstraintFatal
      * @see #setExitOnFirstFatalError
      * @see #setFeature
      */
    virtual void setValidationConstraintFatal(const bool newState) ;
    //@}


    // -----------------------------------------------------------------------
    //  Progressive scan methods
    // -----------------------------------------------------------------------

    /** @name Progressive scan methods */
    //@{

    /** Begin a progressive parse operation
      *
      * This method is used to start a progressive parse on a XML file.
      * To continue parsing, subsequent calls must be to the parseNext
      * method.
      *
      * It scans through the prolog and returns a token to be used on
      * subsequent scanNext() calls. If the return value is true, then the
      * token is legal and ready for further use. If it returns false, then
      * the scan of the prolog failed and the token is not going to work on
      * subsequent scanNext() calls.
      *
      * @param systemId A pointer to a Unicode string represting the path
      *                 to the XML file to be parsed.
      * @param toFill   A token maintaing state information to maintain
      *                 internal consistency between invocation of 'parseNext'
      *                 calls.
      *
      * @return 'true', if successful in parsing the prolog. It indicates the
      *         user can go ahead with parsing the rest of the file. It
      *         returns 'false' to indicate that the parser could parse the
      *         prolog (which means the token will not be valid.)
      *
      * @see #parseNext
      * @see #parseFirst(char*,...)
      * @see #parseFirst(InputSource&,...)
      */
    virtual bool parseFirst
    (
        const   XMLCh* const    systemId
        ,       XMLPScanToken&  toFill
    ) ;

    /** Begin a progressive parse operation
      *
      * This method is used to start a progressive parse on a XML file.
      * To continue parsing, subsequent calls must be to the parseNext
      * method.
      *
      * It scans through the prolog and returns a token to be used on
      * subsequent scanNext() calls. If the return value is true, then the
      * token is legal and ready for further use. If it returns false, then
      * the scan of the prolog failed and the token is not going to work on
      * subsequent scanNext() calls.
      *
      * @param systemId A pointer to a regular native string represting
      *                 the path to the XML file to be parsed.
      * @param toFill   A token maintaing state information to maintain
      *                 internal consIstency between invocation of 'parseNext'
      *                 calls.
      *
      * @return 'true', if successful in parsing the prolog. It indicates the
      *         user can go ahead with parsing the rest of the file. It
      *         returns 'false' to indicate that the parser could not parse
      *         the prolog.
      *
      * @see #parseNext
      * @see #parseFirst(XMLCh*,...)
      * @see #parseFirst(InputSource&,...)
      */
    virtual bool parseFirst
    (
        const   char* const     systemId
        ,       XMLPScanToken&  toFill
    ) ;

    /** Begin a progressive parse operation
      *
      * This method is used to start a progressive parse on a XML file.
      * To continue parsing, subsequent calls must be to the parseNext
      * method.
      *
      * It scans through the prolog and returns a token to be used on
      * subsequent scanNext() calls. If the return value is true, then the
      * token is legal and ready for further use. If it returns false, then
      * the scan of the prolog failed and the token is not going to work on
      * subsequent scanNext() calls.
      *
      * @param source   A const reference to the InputSource object which
      *                 points to the XML file to be parsed.
      * @param toFill   A token maintaing state information to maintain
      *                 internal consistency between invocation of 'parseNext'
      *                 calls.
      *
      * @return 'true', if successful in parsing the prolog. It indicates the
      *         user can go ahead with parsing the rest of the file. It
      *         returns 'false' to indicate that the parser could not parse
      *         the prolog.
      *
      * @see #parseNext
      * @see #parseFirst(XMLCh*,...)
      * @see #parseFirst(char*,...)
      */
    virtual bool parseFirst
    (
        const   InputSource&    source
        ,       XMLPScanToken&  toFill
    ) ;

    /** Continue a progressive parse operation
      *
      * This method is used to continue with progressive parsing of
      * XML files started by a call to 'parseFirst' method.
      *
      * It parses the XML file and stops as soon as it comes across
      * a XML token (as defined in the XML specification). Relevant
      * callback handlers are invoked as required by the SAX
      * specification.
      *
      * @param token A token maintaing state information to maintain
      *              internal consistency between invocation of 'parseNext'
      *              calls.
      *
      * @return 'true', if successful in parsing the next XML token.
      *         It indicates the user can go ahead with parsing the rest
      *         of the file. It returns 'false' to indicate that the parser
      *         could not find next token as per the XML specification
      *         production rule.
      *
      * @see #parseFirst(XMLCh*,...)
      * @see #parseFirst(char*,...)
      * @see #parseFirst(InputSource&,...)
      */
    virtual bool parseNext(XMLPScanToken& token) ;

    /** Reset the parser after a progressive parse
      *
      * If a progressive parse loop exits before the end of the document
      * is reached, the parser has no way of knowing this. So it will leave
      * open any files or sockets or memory buffers that were in use at
      * the time that the parse loop exited.
      *
      * The next parse operation will cause these open files and such to
      * be closed, but the next parse operation might occur at some unknown
      * future point. To avoid this problem, you should reset the parser if
      * you exit the loop early.
      *
      * If you exited because of an error, then this cleanup will be done
      * for you. Its only when you exit the file prematurely of your own
      * accord, because you've found what you wanted in the file most
      * likely.
      *
      * @param token A token maintaing state information to maintain
      *              internal consistency between invocation of 'parseNext'
      *              calls.
      */
    virtual void parseReset(XMLPScanToken& token) ;

    //@}

    // -----------------------------------------------------------------------
    //  Implementation of the grammar preparsing interface
    // -----------------------------------------------------------------------

    /** @name Implementation of Grammar preparsing interface's. */
    //@{
    /**
      * Preparse schema grammar (XML Schema, DTD, etc.) via an input source
      * object.
      *
      * This method invokes the preparsing process on a schema grammar XML
      * file specified by the SAX InputSource parameter.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @param source A const reference to the SAX InputSource object which
      *               points to the schema grammar file to be preparsed.
      * @param grammarType The grammar type (Schema or DTD).
      * @param toCache If <code>true</code>, we cache the preparsed grammar,
      *                otherwise, no chaching. Default is <code>false</code>.
      * @return The preparsed schema grammar object (SchemaGrammar or
      *         DTDGrammar). That grammar object is owned by the parser.
      *
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOMException A DOM exception as per DOM spec.
      *
      * @see InputSource#InputSource
      */
    virtual Grammar* loadGrammar(const InputSource& source,
                                 const short grammarType,
                                 const bool toCache = false);

    /**
      * Preparse schema grammar (XML Schema, DTD, etc.) via a file path or URL
      *
      * This method invokes the preparsing process on a schema grammar XML
      * file specified by the file path parameter.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @param systemId A const XMLCh pointer to the Unicode string which
      *                 contains the path to the XML grammar file to be
      *                 preparsed.
      * @param grammarType The grammar type (Schema or DTD).
      * @param toCache If <code>true</code>, we cache the preparsed grammar,
      *                otherwise, no chaching. Default is <code>false</code>.
      * @return The preparsed schema grammar object (SchemaGrammar or
      *         DTDGrammar). That grammar object is owned by the parser.
      *
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOMException A DOM exception as per DOM spec.
      */
    virtual Grammar* loadGrammar(const XMLCh* const systemId,
                                 const short grammarType,
                                 const bool toCache = false);

    /**
      * Preparse schema grammar (XML Schema, DTD, etc.) via a file path or URL
      *
      * This method invokes the preparsing process on a schema grammar XML
      * file specified by the file path parameter.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @param systemId A const char pointer to a native string which contains
      *                 the path to the XML grammar file to be preparsed.
      * @param grammarType The grammar type (Schema or DTD).
      * @param toCache If <code>true</code>, we cache the preparsed grammar,
      *                otherwise, no chaching. Default is <code>false</code>.
      * @return The preparsed schema grammar object (SchemaGrammar or
      *         DTDGrammar). That grammar object is owned by the parser.
      *
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOMException A DOM exception as per DOM spec.
      */
    virtual Grammar* loadGrammar(const char* const systemId,
                                 const short grammarType,
                                 const bool toCache = false);

    /**
      * Clear the cached grammar pool
      */
    virtual void resetCachedGrammarPool();

    //@}


    // -----------------------------------------------------------------------
    //  Advanced document handler list maintenance methods
    // -----------------------------------------------------------------------

    /** @name Advanced document handler list maintenance methods */
    //@{
    /**
      * This method installs the specified 'advanced' document callback
      * handler, thereby allowing the user to customize the processing,
      * if they choose to do so. Any number of advanced callback handlers
      * maybe installed.
      *
      * <p>The methods in the advanced callback interface represent
      * Xerces-C extensions. There is no specification for this interface.</p>
      *
      * @param toInstall A pointer to the users advanced callback handler.
      *
      * @see #removeAdvDocHandler
      */
    virtual void installAdvDocHandler(XMLDocumentHandler* const toInstall) ;

    /**
      * This method removes the 'advanced' document handler callback from
      * the underlying parser scanner. If no handler is installed, advanced
      * callbacks are not invoked by the scanner.
      * @param toRemove A pointer to the advanced callback handler which
      *                 should be removed.
      *
      * @see #installAdvDocHandler
      */
    virtual bool removeAdvDocHandler(XMLDocumentHandler* const toRemove) ;
    //@}

	// -----------------------------------------------------------------------
    //  Implementation of the XMLDocumentHandler interface
    // -----------------------------------------------------------------------
    /** @name Implementation of the XMLDocumentHandler Interface. */
    //@{
    /**
      * This method is used to report all the characters scanned
      * by the parser. The driver will invoke the 'characters'
      * method of the user installed SAX Document Handler.
      *
      * <p>If any advanced callback handlers are installed, the
      * corresponding 'docCharacters' method will also be invoked.</p>
      *
      * @param chars   A const pointer to a Unicode string representing the
      *                character data.
      * @param length  The length of the Unicode string returned in 'chars'.
      * @param cdataSection  A flag indicating if the characters represent
      *                      content from the CDATA section.
      * @see DocumentHandler#characters
      */
    virtual void docCharacters
    (
        const   XMLCh* const    chars
        , const unsigned int    length
        , const bool            cdataSection
    );

    /**
      * This method is used to report any comments scanned by the parser.
      * This method is a no-op unless, unless an advanced callback handler
      * is installed, in which case the corresponding 'docComment' method
      * is invoked.
      *
      * @param comment A const pointer to a null terminated Unicode
      *                string representing the comment text.
      */
    virtual void docComment
    (
        const   XMLCh* const    comment
    );

    /**
      * This method is used to report any PI scanned by the parser.
      *
      * <p>Any PI's occurring before any 'content' are not reported
      * to any SAX handler as per the specification. However, all
      * PI's within content are reported via the SAX Document Handler's
      * 'processingInstruction' method.
      *
      * <p>If any advanced callback handlers are installed, the
      * corresponding 'docPI' method will be invoked.</p>
      *
      * @param target A const pointer to a Unicode string representing the
      *               target of the PI declaration.
      * @param data   A const pointer to a Unicode string representing the
      *               data of the PI declaration. See the PI production rule
      *               in the XML specification for details.
      *
      * @see DocumentHandler#processingInstruction
      */
    virtual void docPI
    (
        const   XMLCh* const    target
        , const XMLCh* const    data
    );

    /**
      * This method is used to indicate the end of root element
      * was just scanned by the parser. Corresponding 'endDocument'
      * method of the user installed SAX Document Handler will also
      * be invoked.
      *
      * <p>In addition, if any advanced callback handlers are installed,
      * the corresponding 'endDocument' method is invoked.</p>
      *
      * @see DocumentHandler#endDocument
      */
    virtual void endDocument();

    /**
      * This method is used to indicate the end tag of an element.
      * The driver will invoke the corresponding 'endElement' method of
      * the SAX Document Handler interface.
      *
      * <p>If any advanced callback handlers are installed, the
      * corresponding 'endElement' method is also invoked.</p>
      *
      * @param elemDecl A const reference to the object containing element
      *                 declaration information.
      * @param urlId    An id referring to the namespace prefix, if
      *                 namespaces setting is switched on.
      * @param isRoot   A flag indicating whether this element was the
      *                 root element.
      * @param elemPrefix A const pointer to a Unicode string containing
      *                   the namespace prefix for this element. Applicable
      *                   only when namespace processing is enabled.
      * @see DocumentHandler#endElement
      */
    virtual void endElement
    (
        const   XMLElementDecl& elemDecl
        , const unsigned int    urlId
        , const bool            isRoot
        , const XMLCh* const    elemPrefix=0
    );

    /**
      * This method is used to indicate that an end of an entity reference
      * was just scanned.
      *
      * <p>If any advanced callback handlers are installed, the
      * corresponding 'endEnityReference' method is invoked.</p>
      *
      * @param entDecl A const reference to the object containing the
      *                entity declaration information.
      */
    virtual void endEntityReference
    (
        const   XMLEntityDecl&  entDecl
    );

    /**
      * This method is used to report all the whitespace characters,
      * which are determined to be 'ignorable'. This distinction
      * between characters is only made, if validation is enabled.
      * Corresponding 'ignorableWhitespace' method of the user installed
      * SAX Document Handler interface is called.
      *
      * <p>Any whitespace before content is not reported to the SAX
      * Document Handler method, as per the SAX specification.
      * However, if any advanced callback handlers are installed, the
      * corresponding 'ignorableWhitespace' method is invoked.</p>
      *
      * @param chars   A const pointer to a Unicode string representing the
      *                ignorable whitespace character data.
      * @param length  The length of the Unicode string 'chars'.
      * @param cdataSection  A flag indicating if the characters represent
      *                      content from the CDATA section.
      * @see DocumentHandler#ignorableWhitespace
      */
    virtual void ignorableWhitespace
    (
        const   XMLCh* const    chars
        , const unsigned int    length
        , const bool            cdataSection
    );

    /**
      * This method allows the user installed Document Handler and
      * any advanced callback handlers to 'reset' themselves.
      */
    virtual void resetDocument();

    /**
      * This method is used to report the start of the parsing process.
      * The corresponding user installed SAX Document Handler's method
      * 'startDocument' is invoked.
      *
      * <p>If any advanced callback handlers are installed, then the
      * corresponding 'startDocument' method is also called.</p>
      *
      * @see DocumentHandler#startDocument
      */
    virtual void startDocument();

    /**
      * This method is used to report the start of an element. It is
      * called at the end of the element, by which time all attributes
      * specified are also parsed. The corresponding user installed
      * SAX Document Handler's method 'startElement' is invoked.
      *
      * <p>If any advanced callback handlers are installed, then the
      * corresponding 'startElement' method is also called.</p>
      *
      * @param elemDecl A const reference to the object containing element
      *                 declaration information.
      * @param urlId    An id referring to the namespace prefix, if
      *                 namespaces setting is switched on.
      * @param elemPrefix A const pointer to a Unicode string containing
      *                   the namespace prefix for this element. Applicable
      *                   only when namespace processing is enabled.
      * @param attrList  A const reference to the object containing the
      *                  list of attributes just scanned for this element.
      * @param attrCount A count of number of attributes in the list
      *                  specified by the parameter 'attrList'.
      * @param isEmpty  A flag indicating whether this is an empty element
      *                 or not.
      * @param isRoot   A flag indicating whether this element was the
      *                 root element.
      * @see DocumentHandler#startElement
      */
    virtual void startElement
    (
        const   XMLElementDecl&         elemDecl
        , const unsigned int            urlId
        , const XMLCh* const            elemPrefix
        , const RefVectorOf<XMLAttr>&   attrList
        , const unsigned int            attrCount
        , const bool                    isEmpty
        , const bool                    isRoot
    );

    /**
      * This method is used to indicate the start of an entity reference.
      *
      * <p>If any advanced callback handlers are installed, the
      * corresponding 'endEnityReference' method is invoked.</p>
      *
      * @param entDecl A const reference to the object containing the
      *                entity declaration information.
      */
    virtual void startEntityReference
    (
        const   XMLEntityDecl&  entDecl
    );

    /**
      * This method is used to report the XML decl scanned by the parser.
      * Refer to the XML specification to see the meaning of parameters.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      * @param versionStr A const pointer to a Unicode string representing
      *                   version string value.
      * @param encodingStr A const pointer to a Unicode string representing
      *                    the encoding string value.
      * @param standaloneStr A const pointer to a Unicode string
      *                      representing the standalone string value.
      * @param actualEncodingStr A const pointer to a Unicode string
      *                          representing the actual encoding string
      *                          value.
      */
    virtual void XMLDecl
    (
        const   XMLCh* const    versionStr
        , const XMLCh* const    encodingStr
        , const XMLCh* const    standaloneStr
        , const XMLCh* const    actualEncodingStr
    );
    //@}


    // -----------------------------------------------------------------------
    //  Implementation of the XMLErrorReporter interface
    // -----------------------------------------------------------------------

    /** @name Implementation of the XMLErrorReporter Interface. */
    //@{
    /**
      * This method is used to report back errors found while parsing the
      * XML file. The driver will call the corresponding user installed
      * SAX Error Handler methods: 'fatal', 'error', 'warning' depending
      * on the severity of the error. This classification is defined by
      * the XML specification.
      *
      * @param errCode An integer code for the error.
      * @param msgDomain A const pointer to an Unicode string representing
      *                  the message domain to use.
      * @param errType An enumeration classifying the severity of the error.
      * @param errorText A const pointer to an Unicode string representing
      *                  the text of the error message.
      * @param systemId  A const pointer to an Unicode string representing
      *                  the system id of the XML file where this error
      *                  was discovered.
      * @param publicId  A const pointer to an Unicode string representing
      *                  the public id of the XML file where this error
      *                  was discovered.
      * @param lineNum   The line number where the error occurred.
      * @param colNum    The column number where the error occurred.
      * @see ErrorHandler
      */
    virtual void error
    (
        const   unsigned int                errCode
        , const XMLCh* const                msgDomain
        , const XMLErrorReporter::ErrTypes  errType
        , const XMLCh* const                errorText
        , const XMLCh* const                systemId
        , const XMLCh* const                publicId
        , const XMLSSize_t                   lineNum
        , const XMLSSize_t                   colNum
    );

    /**
      * This method allows the user installed Error Handler
      * callback to 'reset' itself.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      */
    virtual void resetErrors();
    //@}


    // -----------------------------------------------------------------------
    //  Implementation of the XMLEntityHandler interface
    // -----------------------------------------------------------------------

    /** @name Implementation of the XMLEntityHandler Interface. */
    //@{
    /**
      * This method is used to indicate the end of parsing of an external
      * entity file.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      * @param inputSource A const reference to the InputSource object
      *                    which points to the XML file being parsed.
      * @see InputSource
      */
    virtual void endInputSource(const InputSource& inputSource);

    /**
      * This method allows an installed XMLEntityHandler to further
      * process any system id's of enternal entities encountered in
      * the XML file being parsed, such as redirection etc.
      *
      * <b><font color="#FF0000">This method always returns 'false'
      * for this SAX driver implementation.</font></b>
      *
      * @param systemId  A const pointer to an Unicode string representing
      *                  the system id scanned by the parser.
      * @param toFill    A pointer to a buffer in which the application
      *                  processed system id is stored.
      * @return 'true', if any processing is done, 'false' otherwise.
      */
    virtual bool expandSystemId
    (
        const   XMLCh* const    systemId
        ,       XMLBuffer&      toFill
    );

    /**
      * This method allows the installed XMLEntityHandler to reset
      * itself.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      */
    virtual void resetEntities();

    /**
      * This method allows a user installed entity handler to further
      * process any pointers to external entities. The applications
      * can implement 'redirection' via this callback. The driver
      * should call the SAX EntityHandler 'resolveEntity' method.
      *
      * @deprecated This method is no longer called (the other resolveEntity one is).
      *
      * @param publicId A const pointer to a Unicode string representing the
      *                 public id of the entity just parsed.
      * @param systemId A const pointer to a Unicode string representing the
      *                 system id of the entity just parsed.
      * @param baseURI  A const pointer to a Unicode string representing the
      *                 base URI of the entity just parsed,
      *                 or <code>null</code> if there is no base URI.
      * @return The value returned by the SAX resolveEntity method or
      *         NULL otherwise to indicate no processing was done.
      *         The returned InputSource is owned by the parser which is
      *         responsible to clean up the memory.
      * @see EntityResolver
      * @see XMLEntityHandler
      */
    virtual InputSource* resolveEntity
    (
        const   XMLCh* const    publicId
        , const XMLCh* const    systemId
        , const XMLCh* const    baseURI = 0
    );

    /** Resolve a public/system id
      *
      * This method allows a user installed entity handler to further
      * process any pointers to external entities. The applications can
      * implement 'redirection' via this callback.  
      *
      * @param resourceIdentifier An object containing the type of
      *        resource to be resolved and the associated data members
      *        corresponding to this type.
      * @return The value returned by the user installed resolveEntity
      *         method or NULL otherwise to indicate no processing was done.
      *         The returned InputSource is owned by the parser which is
      *         responsible to clean up the memory.
      * @see XMLEntityHandler
      * @see XMLEntityResolver
      */
    virtual InputSource* resolveEntity
    (
        XMLResourceIdentifier* resourceIdentifier
    );

    /**
      * This method is used to indicate the start of parsing an
      * external entity file.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      * @param inputSource A const reference to the InputSource object
      *                    which points to the external entity
      *                    being parsed.
      */
    virtual void startInputSource(const InputSource& inputSource);
    //@}

    // -----------------------------------------------------------------------
    //  Implementation of the Deprecated DocTypeHandler Interface
    // -----------------------------------------------------------------------

    /** @name Implementation of the deprecated DocTypeHandler Interface */
    //@{
    /**
      * This method is used to report an attribute definition.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX
      * driver implementation.</font></b>
      *
      * @param elemDecl A const reference to the object containing information
      *                 about the element whose attribute definition was just
      *                 parsed.
      * @param attDef   A const reference to the object containing information
      *                 attribute definition.
      * @param ignore   The flag indicating whether this attribute definition
      *                 was ignored by the parser or not.
      */
    virtual void attDef
    (
        const   DTDElementDecl& elemDecl
        , const DTDAttDef&      attDef
        , const bool            ignoring
    );

    /**
      * This method is used to report a comment occurring within the DTD.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      * @param comment  A const pointer to a Unicode string representing the
      *                 text of the comment just parsed.
      */
    virtual void doctypeComment
    (
        const   XMLCh* const    comment
    );

    /**
      * This method is used to report the DOCTYPE declaration.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      * @param elemDecl A const reference to the object containing information
      *                 about the root element definition declaration of the
      *                 XML document being parsed.
      * @param publicId A const pointer to a Unicode string representing the
      *                 public id of the DTD file.
      * @param systemId A const pointer to a Unicode string representing the
      *                 system id of the DTD file.
      * @param hasIntSubset A flag indicating if this XML file contains any
      *                     internal subset.
      * @param hasExtSubset A flag indicating if this XML file contains any
      *                     external subset. Default is false.
      */
    virtual void doctypeDecl
    (
        const   DTDElementDecl& elemDecl
        , const XMLCh* const    publicId
        , const XMLCh* const    systemId
        , const bool            hasIntSubset
        , const bool            hasExtSubset = false
    );

    /**
      * This method is used to report any PI declarations
      * occurring inside the DTD definition block.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      * @param target A const pointer to a Unicode string representing the
      *               target of the PI declaration.
      * @param data   A const pointer to a Unicode string representing the
      *               data of the PI declaration. See the PI production rule
      *               in the XML specification for details.
      */
    virtual void doctypePI
    (
        const   XMLCh* const    target
        , const XMLCh* const    data
    );

    /**
      * This method is used to report any whitespaces
      * occurring inside the DTD definition block.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      * @param chars  A const pointer to a Unicode string representing the
      *               whitespace characters.
      * @param length The length of the whitespace Unicode string.
      */
    virtual void doctypeWhitespace
    (
        const   XMLCh* const    chars
        , const unsigned int    length
    );

    /**
      * This method is used to report an element declarations
      * successfully scanned by the parser.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      * @param decl   A const reference to the object containing element
      *               declaration information.
      * @param isIgnored The flag indicating whether this definition was
      *                  ignored by the parser or not.
      */
    virtual void elementDecl
    (
        const   DTDElementDecl& decl
        , const bool            isIgnored
    );

    /**
      * This method is used to report the end of an attribute
      * list declaration for an element.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      * @param elemDecl A const reference to the object containing element
      *                 declaration information.
      */
    virtual void endAttList
    (
        const   DTDElementDecl& elemDecl
    );

    /**
      * This method is used to report the end of the internal subset.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      */
    virtual void endIntSubset();

    /**
      * This method is used to report the end of the external subset.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      */
    virtual void endExtSubset();

    /**
      * This method is used to report any entity declarations.
      * For unparsed entities, this driver will invoke the
      * SAX DTDHandler::unparsedEntityDecl callback.
      *
      * @param entityDecl A const reference to the object containing
      *                   the entity declaration information.
      * @param isPEDecl  The flag indicating whether this was a
      *                  parameter entity declaration or not.
      * @param isIgnored The flag indicating whether this definition
      *                  was ignored by the parser or not.
      *
      * @see DTDHandler#unparsedEntityDecl
      */
    virtual void entityDecl
    (
        const   DTDEntityDecl&  entityDecl
        , const bool            isPEDecl
        , const bool            isIgnored
    );

    /**
      * This method allows the user installed DTD handler to
      * reset itself.
      */
    virtual void resetDocType();

    /**
      * This method is used to report any notation declarations.
      * If there is a user installed DTDHandler, then the driver will
      * invoke the SAX DTDHandler::notationDecl callback.
      *
      * @param notDecl A const reference to the object containing the notation
      *                declaration information.
      * @param isIgnored The flag indicating whether this definition was ignored
      *                  by the parser or not.
      *
      * @see DTDHandler#notationDecl
      */
    virtual void notationDecl
    (
        const   XMLNotationDecl&    notDecl
        , const bool                isIgnored
    );

    /**
      * This method is used to indicate the start of an element's attribute
      * list declaration.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      * @param elemDecl A const reference to the object containing element
      *                 declaration information.
      */
    virtual void startAttList
    (
        const   DTDElementDecl& elemDecl
    );

    /**
      * This method is used indicate the start of the internal subset.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      */
    virtual void startIntSubset();

    /**
      * This method is used indicate the start of the external subset.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      */
    virtual void startExtSubset();

    /**
      * This method is used to report the TextDecl. Refer to the XML
      * specification for the syntax of a TextDecl.
      *
      * <b><font color="#FF0000">This method is a no-op for this SAX driver
      * implementation.</font></b>
      *
      * @param versionStr A const pointer to a Unicode string representing
      *                   the version number of the 'version' clause.
      * @param encodingStr A const pointer to a Unicode string representing
      *                    the encoding name of the 'encoding' clause.
      */
    virtual void TextDecl
    (
        const   XMLCh* const    versionStr
        , const XMLCh* const    encodingStr
    );
    //@}


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    SAX2XMLReaderImpl(const SAX2XMLReaderImpl&);
    SAX2XMLReaderImpl& operator=(const SAX2XMLReaderImpl&);

    // -----------------------------------------------------------------------
    //  Initialize/Cleanup methods
    // -----------------------------------------------------------------------
    void initialize();
    void cleanUp();

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fAttrList
    //      A temporary implementation of the basic SAX2 Attributes
    //      interface. We use this one over and over on each startElement
    //      event to allow SAX-like access to the element attributes.
    //
    //  fDocHandler
    //      The installed SAX content handler, if any. Null if none.
    //
    //  fnamespacePrefix
    //      Indicates whether the namespace-prefix feature is on or off.
    //
    //  fautoValidation
    //      Indicates whether automatic validation is on or off
    //
    //  fValidation
    //      Indicates whether the 'validation' core features is on or off
    //
    //  fReuseGrammar
    //      Tells the parser whether it should reuse the grammar or not.
    //      If true, there cannot be any internal subset.
    //
    //	fStringBuffers
    //		Any temporary strings we need are pulled out of this pool
    //
    //	fPrefixes
    //		A Stack of the current namespace prefixes that need calls to
    //		endPrefixMapping
    //
    //	fPrefixCounts
    //		A Stack of the number of prefixes that need endPrefixMapping
    //		calls for that element
    //
    //  fDTDHandler
    //      The installed SAX DTD handler, if any. Null if none.
    //
    //  fElemDepth
    //      This is used to track the element nesting depth, so that we can
    //      know when we are inside content. This is so we can ignore char
    //      data outside of content.
    //
    //  fEntityResolver
    //      The installed SAX entity handler, if any. Null if none.
    //
    //  fErrorHandler
    //      The installed SAX error handler, if any. Null if none.
    //
    //  fLexicalHandler
    //      The installed SAX lexical handler, if any.  Null if none.
    //
    //  fDecllHandler
    //      The installed SAX declaration handler, if any.  Null if none.
    //
    //  fAdvDHCount
    //  fAdvDHList
    //  fAdvDHListSize
    //      This is an array of pointers to XMLDocumentHandlers, which is
    //      how we see installed advanced document handlers. There will
    //      usually not be very many at all, so a simple array is used
    //      instead of a collection, for performance. It will grow if needed,
    //      but that is unlikely.
    //
    //      The count is how many handlers are currently installed. The size
    //      is how big the array itself is (for expansion purposes.) When
    //      count == size, is time to expand.
    //
    //  fParseInProgress
    //      This flag is set once a parse starts. It is used to prevent
    //      multiple entrance or reentrance of the parser.
    //
    //  fScanner
    //      The scanner being used by this parser. It is created internally
    //      during construction.
    //
    //  fHasExternalSubset
    //      Indicate if the document has external DTD subset.
    //
    //   fGrammarPool
    //      The grammar pool passed from external application (through derivatives).
    //      which could be 0, not owned.
    //
    // -----------------------------------------------------------------------
    bool                        fNamespacePrefix;
    bool                        fAutoValidation;
    bool                        fValidation;
    bool                        fParseInProgress;
    bool                        fHasExternalSubset;
    unsigned int                fElemDepth;
    unsigned int                fAdvDHCount;
    unsigned int                fAdvDHListSize;
    VecAttributesImpl		    fAttrList ;
    ContentHandler*		        fDocHandler ;
    RefVectorOf<XMLAttr>*       fTempAttrVec ;
    RefStackOf<XMLBuffer> *     fPrefixes ;
    ValueStackOf<unsigned int>* fPrefixCounts ;
    DTDHandler*                 fDTDHandler;
    EntityResolver*             fEntityResolver;
    XMLEntityResolver*          fXMLEntityResolver;
    ErrorHandler*               fErrorHandler;
    PSVIHandler*                fPSVIHandler;
    LexicalHandler*             fLexicalHandler;
    DeclHandler*                fDeclHandler;
    XMLDocumentHandler**        fAdvDHList;
    XMLScanner*                 fScanner;
    GrammarResolver*            fGrammarResolver;
    XMLStringPool*              fURIStringPool;
    XMLValidator*               fValidator;
    MemoryManager*              fMemoryManager;
    XMLGrammarPool*             fGrammarPool;
    XMLBufferMgr                fStringBuffers;
	
    // -----------------------------------------------------------------------
    // internal function used to set the state of the parser
    // -----------------------------------------------------------------------
    void setValidationScheme(const ValSchemes newScheme);
    void setDoNamespaces(const bool newState);
    bool getDoNamespaces() const;
    void setDoSchema(const bool newState);
    bool getDoSchema() const;
};


// ---------------------------------------------------------------------------
//  SAX2XMLReader: Getter methods
// ---------------------------------------------------------------------------
inline ContentHandler* SAX2XMLReaderImpl::getContentHandler() const
{
    return fDocHandler;
}

inline DTDHandler* SAX2XMLReaderImpl::getDTDHandler() const
{
	return fDTDHandler ;
}

inline EntityResolver* SAX2XMLReaderImpl::getEntityResolver() const
{
	return fEntityResolver;
}

inline XMLEntityResolver* SAX2XMLReaderImpl::getXMLEntityResolver() const
{
	return fXMLEntityResolver;
}

inline ErrorHandler* SAX2XMLReaderImpl::getErrorHandler() const
{
	return fErrorHandler;
}

inline PSVIHandler* SAX2XMLReaderImpl::getPSVIHandler() const
{
	return fPSVIHandler;
}

inline LexicalHandler* SAX2XMLReaderImpl::getLexicalHandler() const
{
   return fLexicalHandler;
}

inline DeclHandler* SAX2XMLReaderImpl::getDeclarationHandler() const
{
   return fDeclHandler;
}

inline bool SAX2XMLReaderImpl::getExitOnFirstFatalError() const
{
    return fScanner->getExitOnFirstFatal();
}

inline bool SAX2XMLReaderImpl::getValidationConstraintFatal() const
{
    return fScanner->getValidationConstraintFatal();
}

inline Grammar* SAX2XMLReaderImpl::getRootGrammar()
{
    return fScanner->getRootGrammar();
}

inline const XMLCh* SAX2XMLReaderImpl::getURIText(unsigned int uriId) const
{
    return fScanner->getURIText(uriId);
}

inline unsigned int SAX2XMLReaderImpl::getSrcOffset() const
{
    return fScanner->getSrcOffset();
}

XERCES_CPP_NAMESPACE_END

#endif
