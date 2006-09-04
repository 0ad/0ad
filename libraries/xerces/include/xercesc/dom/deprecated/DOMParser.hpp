/*
 * Copyright 1999-2002,2004 The Apache Software Foundation.
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
 * $Id: DOMParser.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 *
 */

#if !defined(DOMPARSER_HPP)
#define DOMPARSER_HPP

#include <xercesc/framework/XMLDocumentHandler.hpp>
#include <xercesc/framework/XMLErrorReporter.hpp>
#include <xercesc/framework/XMLEntityHandler.hpp>
#include <xercesc/util/ValueStackOf.hpp>
#include <xercesc/validators/DTD/DocTypeHandler.hpp>
#include <xercesc/validators/DTD/DTDElementDecl.hpp>
#include "DOM_Document.hpp"
#include "DOM_DocumentType.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class EntityResolver;
class ErrorHandler;
class XMLPScanToken;
class XMLScanner;
class XMLValidator;
class Grammar;
class GrammarResolver;
class XMLGrammarPool;
class XMLEntityResolver;
class PSVIHandler;

/**
  * This class implements the Document Object Model (DOM) interface.
  * It should be used by applications which choose to parse and
  * process the XML document using the DOM api's. This implementation
  * also allows the applications to install an error and an entitty
  * handler (useful extensions to the DOM specification).
  *
  * <p>It can be used to instantiate a validating or non-validating
  * parser, by setting a member flag.</p>
  */
class DEPRECATED_DOM_EXPORT DOMParser :

    public XMLDocumentHandler
    , public XMLErrorReporter
    , public XMLEntityHandler
    , public DocTypeHandler
    , public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Class types
    // -----------------------------------------------------------------------
    enum ValSchemes
    {
        Val_Never
        , Val_Always
        , Val_Auto
    };


    // -----------------------------------------------------------------------
    //  Constructors and Detructor
    // -----------------------------------------------------------------------

    /** @name Constructors and Destructor */
    //@{
    /** Construct a DOMParser, with an optional validator
      *
      * Constructor with an instance of validator class to use for
      * validation. If you don't provide a validator, a default one will
      * be created for you in the scanner.
      *
      * @param valToAdopt Pointer to the validator instance to use. The
      *                   parser is responsible for freeing the memory.
      */
    DOMParser
    (
          XMLValidator* const   valToAdopt = 0
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
        , XMLGrammarPool* const gramPool = 0
    );

    /**
      * Destructor
      */
    ~DOMParser();

    //@}

    /** Reset the parser
      *
      * This method resets the state of the DOM driver and makes
      * it ready for a fresh parse run.
      */
    void reset();


    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------

    /** @name Getter methods */
    //@{

    /** Get the DOM document
      *
      * This method returns the DOM_Document object representing the
      * root of the document tree. This object provides the primary
      * access to the document's data.
      *
      * @return The DOM_Document object which represents the entire
      *         XML document.
      */
    DOM_Document getDocument();

    /** Get a pointer to the error handler
      *
      * This method returns the installed error handler. If no handler
      * has been installed, then it will be a zero pointer.
      *
      * @return The pointer to the installed error handler object.
      */
    ErrorHandler* getErrorHandler();

    /** Get a const pointer to the error handler
      *
      * This method returns the installed error handler.  If no handler
      * has been installed, then it will be a zero pointer.
      *
      * @return A const pointer to the installed error handler object.
      */
    const ErrorHandler* getErrorHandler() const;

    /** Get a pointer to the PSVI handler
      *
      * This method returns the installed PSVI handler. If no handler
      * has been installed, then it will be a zero pointer.
      *
      * @return The pointer to the installed PSVI handler object.
      */
    PSVIHandler* getPSVIHandler();

    /** Get a const pointer to the error handler
      *
      * This method returns the installed error handler.  If no handler
      * has been installed, then it will be a zero pointer.
      *
      * @return A const pointer to the installed error handler object.
      */
    const PSVIHandler* getPSVIHandler() const;

    /** Get a pointer to the entity resolver
      *
      * This method returns the installed entity resolver.  If no resolver
      * has been installed, then it will be a zero pointer.
      *
      * @return The pointer to the installed entity resolver object.
      */
    EntityResolver* getEntityResolver();

    /** Get a const pointer to the entity resolver
      *
      * This method returns the installed entity resolver. If no resolver
      * has been installed, then it will be a zero pointer.
      *
      * @return A const pointer to the installed entity resolver object.
      */
    const EntityResolver* getEntityResolver() const;

    /** Get a pointer to the entity resolver
      *
      * This method returns the installed entity resolver.  If no resolver
      * has been installed, then it will be a zero pointer.
      *
      * @return The pointer to the installed entity resolver object.
      */
    XMLEntityResolver* getXMLEntityResolver();

    /** Get a const pointer to the entity resolver
      *
      * This method returns the installed entity resolver. If no resolver
      * has been installed, then it will be a zero pointer.
      *
      * @return A const pointer to the installed entity resolver object.
      */
    const XMLEntityResolver* getXMLEntityResolver() const;

    /** Get a const reference to the underlying scanner
      *
      * This method returns a reference to the underlying scanner object.
      * It allows read only access to data maintained in the scanner.
      *
      * @return A const reference to the underlying scanner object.
      */
    const XMLScanner& getScanner() const;

    /** Get a const reference to the validator
      *
      * This method returns a reference to the parser's installed
      * validator.
      *
      * @return A const reference to the installed validator object.
      */
    const XMLValidator& getValidator() const;

    /**
      * This method returns an enumerated value that indicates the current
      * validation scheme set on this parser.
      *
      * @return The ValSchemes value current set on this parser.
      * @see #setValidationScheme
      */
    ValSchemes getValidationScheme() const;

    /** Get the 'do schema' flag
      *
      * This method returns the state of the parser's schema processing
      * flag.
      *
      * @return true, if the parser is currently configured to
      *         understand schema, false otherwise.
      *
      * @see #setDoSchema
      */
    bool getDoSchema() const;

    /** Get the 'full schema constraint checking' flag
      *
      * This method returns the state of the parser's full schema constraint
      * checking flag.
      *
      * @return true, if the parser is currently configured to
      *         have full schema constraint checking, false otherwise.
      *
      * @see #setValidationSchemaFullChecking
      */
    bool getValidationSchemaFullChecking() const;

    bool getIdentityConstraintChecking() const;

    /** Get error count from the last parse operation.
      *
      * This method returns the error count from the last parse
      * operation. Note that this count is actually stored in the
      * scanner, so this method simply returns what the
      * scanner reports.
      *
      * @return number of errors encountered during the latest
      *			parse operation.
      *
      */
    int getErrorCount() const;

    /** Get the 'do namespaces' flag
      *
      * This method returns the state of the parser's namespace processing
      * flag.
      *
      * @return true, if the parser is currently configured to
      *         understand namespaces, false otherwise.
      *
      * @see #setDoNamespaces
      */
    bool getDoNamespaces() const;

    /** Get the 'exit on first error' flag
      *
      * This method returns the state of the parser's
      * exit-on-First-Fatal-Error flag. If this flag is true, then the
      * parse will exit the first time it sees any non-wellformed XML or
      * any validity error. The default state is true.
      *
      * @return true, if the parser is currently configured to
      *         exit on the first fatal error, false otherwise.
      *
      * @see #setExitOnFirstFatalError
      */
    bool getExitOnFirstFatalError() const;

    /**
      * This method returns the state of the parser's
      * validation-constraint-fatal flag.
      *
      * @return true, if the parser is currently configured to
      *         set validation constraint errors as fatal, false
      *         otherwise.
      *
      * @see #setValidationContraintFatal
      */
    bool getValidationConstraintFatal() const;

    /** Get the 'include entity references' flag
      *
      * This method returns the flag that specifies whether the parser is
      * creating entity reference nodes in the DOM tree being produced.
      *
      * @return  The state of the create entity reference node
      *               flag.
      * @see #setCreateEntityReferenceNodes
      */
    bool  getCreateEntityReferenceNodes()const;

   /** Get the 'include ignorable whitespace' flag.
      *
      * This method returns the state of the parser's include ignorable
      * whitespace flag.
      *
      * @return 'true' if the include ignorable whitespace flag is set on
      *         the parser, 'false' otherwise.
      *
      * @see #setIncludeIgnorableWhitespace
      */
    bool getIncludeIgnorableWhitespace() const;

    /** Get the 'to create MXLDecl node' flag.
      *
      * This method returns the state of the parser's to create XMLDecl
      * DOM Node flag.
      *
      * @return 'true' if the toCreateXMLDeclTypeNode flag is set on
      *         the parser, 'false' otherwise.
      *
      */
    bool getToCreateXMLDeclTypeNode() const;

   /** Get the set of Namespace/SchemaLocation that is specified externaly.
      *
      * This method returns the list of Namespace/SchemaLocation that was
      * specified using setExternalSchemaLocation.
      *
      * The parser owns the returned string, and the memory allocated for
      * the returned string will be destroyed when the parser is deleted.
      *
      * To ensure assessiblity of the returned information after the parser
      * is deleted, callers need to copy and store the returned information
      * somewhere else.
      *
      * @return a pointer to the list of Namespace/SchemaLocation that was
      *         specified externally.  The pointer spans the same life-time as
      *         the parser.  A null pointer is returned if nothing
      *         was specified externally.
      *
      * @see #setExternalSchemaLocation(const XMLCh* const)
      */
    XMLCh* getExternalSchemaLocation() const;

   /** Get the noNamespace SchemaLocation that is specified externaly.
      *
      * This method returns the no target namespace XML Schema Location
      * that was specified using setExternalNoNamespaceSchemaLocation.
      *
      * The parser owns the returned string, and the memory allocated for
      * the returned string will be destroyed when the parser is deleted.
      *
      * To ensure assessiblity of the returned information after the parser
      * is deleted, callers need to copy and store the returned information
      * somewhere else.
      *
      * @return a pointer to the no target namespace Schema Location that was
      *         specified externally.  The pointer spans the same life-time as
      *         the parser.  A null pointer is returned if nothing
      *         was specified externally.
      *
      * @see #setExternalNoNamespaceSchemaLocation(const XMLCh* const)
      */
    XMLCh* getExternalNoNamespaceSchemaLocation() const;

    /** Get the 'Grammar caching' flag
      *
      * This method returns the state of the parser's grammar caching when
      * parsing an XML document.
      *
      * @return true, if the parser is currently configured to
      *         cache grammars, false otherwise.
      *
      * @see #cacheGrammarFromParse
      */
    bool isCachingGrammarFromParse() const;

    /** Get the 'Use cached grammar' flag
      *
      * This method returns the state of the parser's use of cached grammar
      * when parsing an XML document.
      *
      * @return true, if the parser is currently configured to
      *         use cached grammars, false otherwise.
      *
      * @see #useCachedGrammarInParse
      */
    bool isUsingCachedGrammarInParse() const;

    /**
      * Get the 'calculate src offset flag'
      *
      * This method returns the state of the parser's src offset calculation
      * when parsing an XML document.
      *
      * @return true, if the parser is currently configured to
      *         calculate src offsets, false otherwise.
      *
      * @see #setCalculateSrcOfs
      */
    bool getCalculateSrcOfs() const;

    /**
      * Get the 'force standard uri flag'
      *
      * This method returns the state if the parser forces standard uri
      *
      * @return true, if the parser is currently configured to
      *         force standard uri, i.e. malformed uri will be rejected.
      *
      * @see #setStandardUriConformant
      */
    bool getStandardUriConformant() const;

    /**
      * Retrieve the grammar that is associated with the specified namespace key
      *
      * @param  nameSpaceKey Namespace key
      * @return Grammar associated with the Namespace key.
      */
    Grammar* getGrammar(const XMLCh* const nameSpaceKey);

    /**
      * Retrieve the grammar where the root element is declared.
      *
      * @return Grammar where root element declared
      */
    Grammar* getRootGrammar();

    /**
      * Returns the string corresponding to a URI id from the URI string pool.
      *
      * @param uriId id of the string in the URI string pool.
      * @return URI string corresponding to the URI id.
      */
    const XMLCh* getURIText(unsigned int uriId) const;

    /**
      * Returns the current src offset within the input source.
      *
      * @return offset within the input source
      */
    unsigned int getSrcOffset() const;

    //@}


    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------

    /** @name Setter methods */
    //@{

    /** Set the error handler
      *
      * This method allows applications to install their own error handler
      * to trap error and warning messages.
      *
      * <i>Any previously set handler is merely dropped, since the parser
      * does not own them.</i>
      *
      * @param handler  A const pointer to the user supplied error
      *                 handler.
      *
      * @see #getErrorHandler
      */
    void setErrorHandler(ErrorHandler* const handler);

    /** Set the PSVI handler
      *
      * This method allows applications to install their own PSVI handler.
      *
      * <i>Any previously set handler is merely dropped, since the parser
      * does not own them.</i>
      *
      * @param handler  A const pointer to the user supplied PSVI
      *                 handler.
      *
      * @see #getPSVIHandler
      */
    void setPSVIHandler(PSVIHandler* const handler);


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
      * @param handler  A const pointer to the user supplied entity
      *                 resolver.
      *
      * @see #getEntityResolver
      */
    void setEntityResolver(EntityResolver* const handler);

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
      * @param handler  A const pointer to the user supplied entity
      *                 resolver.
      *
      * @see #getXMLEntityResolver
      */
    void setXMLEntityResolver(XMLEntityResolver* const handler);

    /** Set the 'do namespaces' flag
      *
      * This method allows users to enable or disable the parser's
      * namespace processing. When set to true, parser starts enforcing
      * all the constraints and rules specified by the NameSpace
      * specification.
      *
      * The parser's default state is: false.
      *
      * @param newState The value specifying whether NameSpace rules should
      *                 be enforced or not.
      *
      * @see #getDoNamespaces
      */
    void setDoNamespaces(const bool newState);

    /** Set the 'exit on first error' flag
      *
      * This method allows users to set the parser's behaviour when it
      * encounters the first fatal error. If set to true, the parser
      * will exit at the first fatal error. If false, then it will
      * report the error and continue processing.
      *
      * The default value is 'true' and the parser exits on the
      * first fatal error.
      *
      * @param newState The value specifying whether the parser should
      *                 continue or exit when it encounters the first
      *                 fatal error.
      *
      * @see #getExitOnFirstFatalError
      */
    void setExitOnFirstFatalError(const bool newState);

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
      * @param newState If true, the parser will exit if "setExitOnFirstFatalError"
      *                 is set to true.
      *
      * @see #getValidationConstraintFatal
      * @see #setExitOnFirstFatalError
      */
    void setValidationConstraintFatal(const bool newState);

     /** Set the 'include entity references' flag
      *
      * This method allows the user to specify whether the parser should
      * create entity reference nodes in the DOM tree being produced.
      * When the 'create' flag is
      * true, the parser will create EntityReference nodes in the DOM tree.
      * The EntityReference nodes and their child nodes will be read-only.
      * When the 'create' flag is false, no EntityReference nodes will be created.
      * <p>The replacement text
      * of the entity is included in either case, either as a
      * child of the Entity Reference node or in place at the location
      * of the reference.
      * <p>The default value is 'true'.
      *
      * @param create The new state of the create entity reference nodes
      *               flag.
      * @see #getCreateEntityReferenceNodes
      */
    void setCreateEntityReferenceNodes(const bool create);

   /** Set the 'include ignorable whitespace' flag
      *
      * This method allows the user to specify whether a validating parser
      * should include ignorable whitespaces as text nodes.  It has no effect
      * on non-validating parsers which always include non-markup text.
      * <p>When set to true (also the default), ignorable whitespaces will be
      * added to the DOM tree as text nodes.  The method
      * DOM_Text::isIgnorableWhitespace() will return true for those text
      * nodes only.
      * <p>When set to false, all ignorable whitespace will be discarded and
      * no text node is added to the DOM tree.  Note: applications intended
      * to process the "xml:space" attribute should not set this flag to false.
      * And this flag also overrides any schema datateye whitespace facets,
      * that is, all ignorable whitespace will be discarded even though
      * 'preserve' is set in schema datatype whitespace facets.
      *
      * @param include The new state of the include ignorable whitespace
      *                flag.
      *
      * @see #getIncludeIgnorableWhitespace
      */
    void setIncludeIgnorableWhitespace(const bool include);

    /**
      * This method allows users to set the validation scheme to be used
      * by this parser. The value is one of the ValSchemes enumerated values
      * defined by this class:
      *
      * <br>  Val_Never  - turn off validation
      * <br>  Val_Always - turn on validation
      * <br>  Val_Auto   - turn on validation if any internal/external
      *                  DTD subset have been seen
      *
      * <p>The parser's default state is: Val_Auto.</p>
      *
      * @param newScheme The new validation scheme to use.
      *
      * @see #getValidationScheme
      */
    void setValidationScheme(const ValSchemes newScheme);

    /** Set the 'do schema' flag
      *
      * This method allows users to enable or disable the parser's
      * schema processing. When set to false, parser will not process
      * any schema found.
      *
      * The parser's default state is: false.
      *
      * Note: If set to true, namespace processing must also be turned on.
      *
      * @param newState The value specifying whether schema support should
      *                 be enforced or not.
      *
      * @see #getDoSchema
      */
    void setDoSchema(const bool newState);

    /**
      * This method allows the user to turn full Schema constraint checking on/off.
      * Only takes effect if Schema validation is enabled.
      * If turned off, partial constraint checking is done.
      *
      * Full schema constraint checking includes those checking that may
      * be time-consuming or memory intensive. Currently, particle unique
      * attribution constraint checking and particle derivation resriction checking
      * are controlled by this option.
      *
      * The parser's default state is: false.
      *
      * @param schemaFullChecking True to turn on full schema constraint checking.
      *
      * @see #getValidationSchemaFullChecking
      */
    void setValidationSchemaFullChecking(const bool schemaFullChecking);

    void setIdentityConstraintChecking(const bool identityConstraintChecking);

    /**
      * This method allows users to set the toCreateXMLDeclTypeNode flag
      * by this parser. By setting it to 'true' user can have XMLDecl type
      * nodes attached to the DOM tree.
      *
      * <p>The parser's default state is: false </p>
      *
      * @param create The new to create XMLDecl type node flag
      *
      */
    void setToCreateXMLDeclTypeNode(const bool create);

    /**
      * This method allows the user to specify a list of schemas to use.
      * If the targetNamespace of a schema specified using this method matches
      * the targetNamespace of a schema occuring in the instance document in
      * the schemaLocation attribute, or if the targetNamespace matches the
      * namespace attribute of the "import" element, the schema specified by the
      * user using this method will be used (i.e., the schemaLocation attribute
      * in the instance document or on the "import" element will be effectively ignored).
      *
      * If this method is called more than once, only the last one takes effect.
      *
      * The syntax is the same as for schemaLocation attributes in instance
      * documents: e.g, "http://www.example.com file_name.xsd". The user can
      * specify more than one XML Schema in the list.
      *
      * @param schemaLocation the list of schemas to use
      *
      * @see #getExternalSchemaLocation
      */

    void setExternalSchemaLocation(const XMLCh* const schemaLocation);

    /**
      * This method is same as setExternalSchemaLocation(const XMLCh* const).
      * It takes native char string as parameter
      *
      * @param schemaLocation the list of schemas to use
      *
      * @see #setExternalSchemaLocation(const XMLCh* const)
      */
    void setExternalSchemaLocation(const char* const schemaLocation);

    /**
      * This method allows the user to specify the no target namespace XML
      * Schema Location externally.  If specified, the instance document's
      * noNamespaceSchemaLocation attribute will be effectively ignored.
      *
      * If this method is called more than once, only the last one takes effect.
      *
      * The syntax is the same as for the noNamespaceSchemaLocation attribute
      * that may occur in an instance document: e.g."file_name.xsd".
      *
      * @param noNamespaceSchemaLocation the XML Schema Location with no target namespace
      *
      * @see #getExternalNoNamespaceSchemaLocation
      */
    void setExternalNoNamespaceSchemaLocation(const XMLCh* const noNamespaceSchemaLocation);

    /**
      * This method is same as setExternalNoNamespaceSchemaLocation(const XMLCh* const).
      * It takes native char string as parameter
      *
      * @param noNamespaceSchemaLocation the XML Schema Location with no target namespace
      *
      * @see #setExternalNoNamespaceSchemaLocation(const XMLCh* const)
      */
    void setExternalNoNamespaceSchemaLocation(const char* const noNamespaceSchemaLocation);

    /** Set the 'Grammar caching' flag
      *
      * This method allows users to enable or disable caching of grammar when
      * parsing XML documents. When set to true, the parser will cache the
      * resulting grammar for use in subsequent parses.
      *
      * If the flag is set to true, the 'Use cached grammar' flag will also be
      * set to true.
      *
      * The parser's default state is: false.
      *
      * @param newState The value specifying whether we should cache grammars
      *                 or not.
      *
      * @see #isCachingGrammarFromParse
      * @see #useCachedGrammarInParse
      */
    void cacheGrammarFromParse(const bool newState);

    /** Set the 'Use cached grammar' flag
      *
      * This method allows users to enable or disable the use of cached
      * grammars.  When set to true, the parser will use the cached grammar,
      * instead of building the grammar from scratch, to validate XML
      * documents.
      *
      * If the 'Grammar caching' flag is set to true, this mehod ignore the
      * value passed in.
      *
      * The parser's default state is: false.
      *
      * @param newState The value specifying whether we should use the cached
      *                 grammar or not.
      *
      * @see #isUsingCachedGrammarInParse
      * @see #cacheGrammarFromParse
      */
    void useCachedGrammarInParse(const bool newState);

    /** Enable/disable src offset calculation
      *
      * This method allows users to enable/disable src offset calculation.
      * Disabling the calculation will improve performance.
      *
      * The parser's default state is: false.
      *
      * @param newState The value specifying whether we should enable or
      *                 disable src offset calculation
      *
      * @see #getCalculateSrcOfs
      */
    void setCalculateSrcOfs(const bool newState);

    /** Force standard uri
      *
      * This method allows users to tell the parser to force standard uri conformance.
      *
      * The parser's default state is: false.
      *
      * @param newState The value specifying whether the parser should reject malformed URI.
      *
      * @see #getStandardUriConformant
      */
    void setStandardUriConformant(const bool newState);

    /** Set the scanner to use when scanning the XML document
      *
      * This method allows users to set the scanner to use
      * when scanning a given XML document.
      *
      * @param scannerName The name of the desired scanner
      */
    void useScanner(const XMLCh* const scannerName);

    //@}


    // -----------------------------------------------------------------------
    //  Parsing methods
    // -----------------------------------------------------------------------

    /** @name Parsing methods */
    //@{

    /** Parse via an input source object
      *
      * This method invokes the parsing process on the XML file specified
      * by the InputSource parameter. This API is borrowed from the
      * SAX Parser interface.
      *
      * @param source A const reference to the InputSource object which
      *               points to the XML file to be parsed.
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOM_DOMException A DOM exception as per DOM spec.
      * @see InputSource#InputSource
      * @see #setEntityResolver
      * @see #setErrorHandler
      */
    void parse(const InputSource& source);

    /** Parse via a file path or URL
      *
      * This method invokes the parsing process on the XML file specified by
      * the Unicode string parameter 'systemId'. This method is borrowed
      * from the SAX Parser interface.
      *
      * @param systemId A const XMLCh pointer to the Unicode string which
      *                 contains the path to the XML file to be parsed.
      *
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOM_DOMException A DOM exception as per DOM spec.
      * @see #parse(InputSource,...)
      */
    void parse(const XMLCh* const systemId);

    /** Parse via a file path or URL (in the local code page)
      *
      * This method invokes the parsing process on the XML file specified by
      * the native char* string parameter 'systemId'.
      *
      * @param systemId A const char pointer to a native string which
      *                 contains the path to the XML file to be parsed.
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOM_DOMException A DOM exception as per DOM spec.
      * @see #parse(InputSource,...)
      */
    void parse(const char* const systemId);

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
      * @return 'true', if successful in parsing the prolog. It indicates the
      *         user can go ahead with parsing the rest of the file. It
      *         returns 'false' to indicate that the parser could not parse
      *         the prolog.
      *
      * @see #parseNext
      * @see #parseFirst(char*,...)
      * @see #parseFirst(InputSource&,...)
      */
    bool parseFirst
    (
        const   XMLCh* const    systemId
        ,       XMLPScanToken&  toFill
    );

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
      * @see #parseFirst(InputSource&,...)
      */
    bool parseFirst
    (
        const   char* const     systemId
        ,       XMLPScanToken&  toFill
    );

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
    bool parseFirst
    (
        const   InputSource&    source
        ,       XMLPScanToken&  toFill
    );

    /** Continue a progressive parse operation
      *
      * This method is used to continue with progressive parsing of
      * XML files started by a call to 'parseFirst' method.
      *
      * It parses the XML file and stops as soon as it comes across
      * a XML token (as defined in the XML specification).
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
    bool parseNext(XMLPScanToken& token);

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
      *
      * @see #parseFirst(XMLCh*,...)
      * @see #parseFirst(char*,...)
      * @see #parseFirst(InputSource&,...)
      */
    void parseReset(XMLPScanToken& token);

    //@}

    // -----------------------------------------------------------------------
    //  Grammar preparsing interface
    // -----------------------------------------------------------------------

    /** @name Implementation of Grammar preparsing interface's. */
    //@{
    /**
      * Preparse schema grammar (XML Schema, DTD, etc.) via an input source
      * object.
      *
      * This method invokes the preparsing process on a schema grammar XML
      * file specified by the SAX InputSource parameter. If the 'toCache' flag
      * is enabled, the parser will cache the grammars for re-use. If a grammar
      * key is found in the pool, no caching of any grammar will take place.
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
    Grammar* loadGrammar(const InputSource& source,
                         const short grammarType,
                         const bool toCache = false);

    /**
      * Preparse schema grammar (XML Schema, DTD, etc.) via a file path or URL
      *
      * This method invokes the preparsing process on a schema grammar XML
      * file specified by the file path parameter. If the 'toCache' flag
      * is enabled, the parser will cache the grammars for re-use. If a grammar
      * key is found in the pool, no caching of any grammar will take place.
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
    Grammar* loadGrammar(const XMLCh* const systemId,
                         const short grammarType,
                         const bool toCache = false);

    /**
      * Preparse schema grammar (XML Schema, DTD, etc.) via a file path or URL
      *
      * This method invokes the preparsing process on a schema grammar XML
      * file specified by the file path parameter. If the 'toCache' flag
      * is enabled, the parser will cache the grammars for re-use. If a grammar
      * key is found in the pool, no caching of any grammar will take place.
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
    Grammar* loadGrammar(const char* const systemId,
                         const short grammarType,
                         const bool toCache = false);

    /**
      * This method allows the user to reset the pool of cached grammars.
      */
    void resetCachedGrammarPool();

    //@}



    // -----------------------------------------------------------------------
    //  Implementation of the XMLErrorReporter interface.
    // -----------------------------------------------------------------------

    /** @name Implementation of the XMLErrorReporter interface. */
    //@{

    /** Handle errors reported from the parser
      *
      * This method is used to report back errors found while parsing the
      * XML file. This method is also borrowed from the SAX specification.
      * It calls the corresponding user installed Error Handler method:
      * 'fatal', 'error', 'warning' depending on the severity of the error.
      * This classification is defined by the XML specification.
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
        , const XMLSSize_t                  lineNum
        , const XMLSSize_t                  colNum
    );

    /** Reset any error data before a new parse
     *
      * This method allows the user installed Error Handler callback to
      * 'reset' itself.
      *
      * <b><font color="#FF0000">This method is a no-op for this DOM
      * implementation.</font></b>
      */
    virtual void resetErrors();
    //@}


    // -----------------------------------------------------------------------
    //  Implementation of the XMLEntityHandler interface.
    // -----------------------------------------------------------------------

    /** @name Implementation of the XMLEntityHandler interface. */
    //@{

    /** Handle an end of input source event
      *
      * This method is used to indicate the end of parsing of an external
      * entity file.
      *
      * <b><font color="#FF0000">This method is a no-op for this DOM
      * implementation.</font></b>
      *
      * @param inputSource A const reference to the InputSource object
      *                    which points to the XML file being parsed.
      * @see InputSource
      */
    virtual void endInputSource(const InputSource& inputSource);

    /** Expand a system id
      *
      * This method allows an installed XMLEntityHandler to further
      * process any system id's of enternal entities encountered in
      * the XML file being parsed, such as redirection etc.
      *
      * <b><font color="#FF0000">This method always returns 'false'
      * for this DOM implementation.</font></b>
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

    /** Reset any entity handler information
      *
      * This method allows the installed XMLEntityHandler to reset
      * itself.
      *
      * <b><font color="#FF0000">This method is a no-op for this DOM
      * implementation.</font></b>
      */
    virtual void resetEntities();

    /** Resolve a public/system id
      *
      * This method allows a user installed entity handler to further
      * process any pointers to external entities. The applications can
      * implement 'redirection' via this callback. This method is also
      * borrowed from the SAX specification.
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
      * @return The value returned by the user installed resolveEntity
      *         method or NULL otherwise to indicate no processing was done.
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

    /** Handle a 'start input source' event
      *
      * This method is used to indicate the start of parsing an external
      * entity file.
      *
      * <b><font color="#FF0000">This method is a no-op for this DOM parse
      * implementation.</font></b>
      *
      * @param inputSource A const reference to the InputSource object
      *                    which points to the external entity
      *                    being parsed.
      */
    virtual void startInputSource(const InputSource& inputSource);

    //@}



    // -----------------------------------------------------------------------
    //  Implementation of the XMLDocumentHandler interface.
    // -----------------------------------------------------------------------

    /** @name Implementation of the XMLDocumentHandler interface. */
    //@{

    /** Handle document character events
      *
      * This method is used to report all the characters scanned by the
      * parser. This DOM implementation stores this data in the appropriate
      * DOM node, creating one if necessary.
      *
      * @param chars   A const pointer to a Unicode string representing the
      *                character data.
      * @param length  The length of the Unicode string returned in 'chars'.
      * @param cdataSection  A flag indicating if the characters represent
      *                      content from the CDATA section.
      */
    virtual void docCharacters
    (
        const   XMLCh* const    chars
        , const unsigned int    length
        , const bool            cdataSection
    );

    /** Handle a document comment event
      *
      * This method is used to report any comments scanned by the parser.
      * A new comment node is created which stores this data.
      *
      * @param comment A const pointer to a null terminated Unicode
      *                string representing the comment text.
      */
    virtual void docComment
    (
        const   XMLCh* const    comment
    );

    /** Handle a document PI event
      *
      * This method is used to report any PI scanned by the parser. A new
      * PI node is created and appended as a child of the current node in
      * the tree.
      *
      * @param target A const pointer to a Unicode string representing the
      *               target of the PI declaration.
      * @param data   A const pointer to a Unicode string representing the
      *               data of the PI declaration. See the PI production rule
      *               in the XML specification for details.
      */
    virtual void docPI
    (
        const   XMLCh* const    target
        , const XMLCh* const    data
    );

    /** Handle the end of document event
      *
      * This method is used to indicate the end of the current document.
      */
    virtual void endDocument();

    /** Handle and end of element event
      *
      * This method is used to indicate the end tag of an element. The
      * DOMParse pops the current element off the top of the element
      * stack, and make it the new current element.
      *
      * @param elemDecl A const reference to the object containing element
      *                 declaration information.
      * @param urlId    An id referring to the namespace prefix, if
      *                 namespaces setting is switched on.
      * @param isRoot   A flag indicating whether this element was the
      *                 root element.
      * @param elemPrefix A const pointer to a Unicode string containing
      *                 the namespace prefix for this element. Applicable
      *                 only when namespace processing is enabled.
      */
    virtual void endElement
    (
        const   XMLElementDecl& elemDecl
        , const unsigned int    urlId
        , const bool            isRoot
        , const XMLCh* const    elemPrefix=0
    );

    /** Handle and end of entity reference event
      *
      * This method is used to indicate that an end of an entity reference
      * was just scanned.
      *
      * @param entDecl A const reference to the object containing the
      *                entity declaration information.
      */
    virtual void endEntityReference
    (
        const   XMLEntityDecl&  entDecl
    );

    /** Handle an ignorable whitespace vent
      *
      * This method is used to report all the whitespace characters, which
      * are determined to be 'ignorable'. This distinction between characters
      * is only made, if validation is enabled.
      *
      * Any whitespace before content is ignored. If the current node is
      * already of type DOM_Node::TEXT_NODE, then these whitespaces are
      * appended, otherwise a new Text node is created which stores this
      * data. Essentially all contiguous ignorable characters are collected
      * in one node.
      *
      * @param chars   A const pointer to a Unicode string representing the
      *                ignorable whitespace character data.
      * @param length  The length of the Unicode string 'chars'.
      * @param cdataSection  A flag indicating if the characters represent
      *                      content from the CDATA section.
      */
    virtual void ignorableWhitespace
    (
        const   XMLCh* const    chars
        , const unsigned int    length
        , const bool            cdataSection
    );

    /** Handle a document reset event
      *
      * This method allows the user installed Document Handler to 'reset'
      * itself, freeing all the memory resources. The scanner calls this
      * method before starting a new parse event.
      */
    virtual void resetDocument();

    /** Handle a start document event
      *
      * This method is used to report the start of the parsing process.
      */
    virtual void startDocument();

    /** Handle a start element event
      *
      * This method is used to report the start of an element. It is
      * called at the end of the element, by which time all attributes
      * specified are also parsed. A new DOM Element node is created
      * along with as many attribute nodes as required. This new element
      * is added appended as a child of the current node in the tree, and
      * then replaces it as the current node (if the isEmpty flag is false.)
      *
      * @param elemDecl A const reference to the object containing element
      *                 declaration information.
      * @param urlId    An id referring to the namespace prefix, if
      *                 namespaces setting is switched on.
      * @param elemPrefix A const pointer to a Unicode string containing
      *                 the namespace prefix for this element. Applicable
      *                 only when namespace processing is enabled.
      * @param attrList A const reference to the object containing the
      *                 list of attributes just scanned for this element.
      * @param attrCount A count of number of attributes in the list
      *                 specified by the parameter 'attrList'.
      * @param isEmpty  A flag indicating whether this is an empty element
      *                 or not. If empty, then no endElement() call will
      *                 be made.
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

    /** Handle a start entity reference event
      *
      * This method is used to indicate the start of an entity reference.
      * If the expand entity reference flag is true, then a new
      * DOM Entity reference node is created.
      *
      * @param entDecl A const reference to the object containing the
      *                entity declaration information.
      */
    virtual void startEntityReference
    (
        const   XMLEntityDecl&  entDecl
    );

    /** Handle an XMLDecl event
      *
      * This method is used to report the XML decl scanned by the parser.
      * Refer to the XML specification to see the meaning of parameters.
      *
      * <b><font color="#FF0000">This method is a no-op for this DOM
      * implementation.</font></b>
      *
      * @param versionStr A const pointer to a Unicode string representing
      *                   version string value.
      * @param encodingStr A const pointer to a Unicode string representing
      *                    the encoding string value.
      * @param standaloneStr A const pointer to a Unicode string
      *                      representing the standalone string value.
      * @param actualEncStr A const pointer to a Unicode string
      *                     representing the actual encoding string
      *                     value.
      */
    virtual void XMLDecl
    (
        const   XMLCh* const    versionStr
        , const XMLCh* const    encodingStr
        , const XMLCh* const    standaloneStr
        , const XMLCh* const    actualEncStr
    );
    //@}


    /** @name Deprecated Methods */
    //@{
    /** Set the 'expand entity references' flag
      *
      * DEPRECATED.  USE setCreateEntityReferenceNodes instead.
      * This method allows the user to specify whether the parser should
      * expand all entity reference nodes. When the 'do expansion' flag is
      * true, the DOM tree does not have any entity reference nodes. It is
      * replaced by the sub-tree representing the replacement text of the
      * entity. When the 'do expansion' flag is false, the DOM tree
      * contains an extra entity reference node, whose children is the
      * sub tree of the replacement text.
      * <p>The default value is 'false'.
      *
      * @param expand The new state of the expand entity reference
      *               flag.
      * @see #setCreateEntityReferenceNodes
      */
    void setExpandEntityReferences(const bool expand);

    /** Get the 'expand entity references' flag.
      * DEPRECATED Use getCreateEntityReferenceNodes() instead.
      *
      * This method returns the state of the parser's expand entity
      * references flag.
      *
      * @return 'true' if the expand entity reference flag is set on
      *         the parser, 'false' otherwise.
      *
      * @see #setExpandEntityReferences
      * @see #setCreateEntityReferenceNodes
      * @see #getCreateEntityReferenceNodes
      */
    bool getExpandEntityReferences() const;

    /**
      * DEPRECATED Use getValidationScheme() instead
      *
      * This method returns the state of the parser's validation
      * handling flag which controls whether validation checks
      * are enforced or not.
      *
      * @return true, if the parser is currently configured to
      *         do validation, false otherwise.
      *
      * @see #setDoValidation
      * @see #getValidationScheme
      */
    bool getDoValidation() const;

    /**
      * DEPRECATED Use setValidationScheme(const ValSchemes newScheme) instead
      *
      * This method allows users to enable or disable the parser's validation
      * checks.
      *
      * <p>By default, the parser does not to any validation. The default
      * value is false.</p>
      *
      * @param newState The value specifying whether the parser should
      *                 do validity checks or not against the DTD in the
      *                 input XML document.
      *
      * @see #getDoValidation
      * @see #setValidationScheme
      */
    void setDoValidation(const bool newState);

    /**
      * Deprecated doctypehandler interfaces
      */
	virtual void attDef
    (
        const   DTDElementDecl&     elemDecl
        , const DTDAttDef&          attDef
        , const bool                ignoring
    );

    virtual void doctypeComment
    (
        const   XMLCh* const    comment
    );

    virtual void doctypeDecl
    (
        const   DTDElementDecl& elemDecl
        , const XMLCh* const    publicId
        , const XMLCh* const    systemId
        , const bool            hasIntSubset
        , const bool            hasExtSubset = false
    );

    virtual void doctypePI
    (
        const   XMLCh* const    target
        , const XMLCh* const    data
    );

    virtual void doctypeWhitespace
    (
        const   XMLCh* const    chars
        , const unsigned int    length
    );

    virtual void elementDecl
    (
        const   DTDElementDecl& decl
        , const bool            isIgnored
    );

    virtual void endAttList
    (
        const   DTDElementDecl& elemDecl
    );

    virtual void endIntSubset();

    virtual void endExtSubset();

    virtual void entityDecl
    (
        const   DTDEntityDecl&  entityDecl
        , const bool            isPEDecl
        , const bool            isIgnored
    );

    virtual void resetDocType();

    virtual void notationDecl
    (
        const   XMLNotationDecl&    notDecl
        , const bool                isIgnored
    );

    virtual void startAttList
    (
        const   DTDElementDecl& elemDecl
    );

    virtual void startIntSubset();

    virtual void startExtSubset();

    virtual void TextDecl
    (
        const   XMLCh* const    versionStr
        , const XMLCh* const    encodingStr
    );


    //@}


protected :
    // -----------------------------------------------------------------------
    //  Protected getter methods
    // -----------------------------------------------------------------------

    /** @name Protected getter methods */
    //@{
    /** Get the current DOM node
      *
      * This provides derived classes with access to the current node, i.e.
      * the node to which new nodes are being added.
      */
    DOM_Node getCurrentNode();

    //@}


    // -----------------------------------------------------------------------
    //  Protected setter methods
    // -----------------------------------------------------------------------

    /** @name Protected setter methods */
    //@{

    /** Set the current DOM node
      *
      * This method sets the current node maintained inside the parser to
      * the one specified.
      *
      * @param toSet The DOM node which will be the current node.
      */
    void setCurrentNode(DOM_Node toSet);

    /** Set the document node
      *
      * This method sets the DOM Document node to the one specified.
      *
      * @param toSet The new DOM Document node for this XML document.
      */
    void setDocument(DOM_Document toSet);
    //@}


private :
    // -----------------------------------------------------------------------
    //  Protected setter methods
    // -----------------------------------------------------------------------
    void initialize();
    void cleanUp();

    // unimplemented
    DOMParser ( const DOMParser& toCopy);
    DOMParser& operator= (const DOMParser& other);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fCurrentNode
    //  fCurrentParent
    //      Used to track the current node during nested element events. Since
    //      the tree must be built from a set of disjoint callbacks, we need
    //      these to keep up with where we currently are.
    //
    //  fDocument
    //      The root document object, filled with the document contents.
    //
    //  fEntityResolver
    //      The installed SAX entity resolver, if any. Null if none.
    //
    //  fErrorHandler
    //      The installed SAX error handler, if any. Null if none.
    //
    //  fCreateEntityReferenceNode
    //      Indicates whether entity reference nodes should be created.
    //
    //  fIncludeIgnorableWhitespace
    //      Indicates whether ignorable whiltespace should be added to
    //      the DOM tree for validating parsers.
    //
    //  fNodeStack
    //      Used to track previous parent nodes during nested element events.
    //
    //  fParseInProgress
    //      Used to prevent multiple entrance to the parser while its doing
    //      a parse.
    //
    //  fScanner
    //      The scanner used for this parser. This is created during the
    //      constructor.
    //
    //  fWithinElement
    //      A flag to indicate that the parser is within at least one level
    //      of element processing.
    //
    //  fDocumentType
    //      Used to store and update the documentType variable information
    //      in fDocument
    //
    //  fToCreateXMLDecTypeNode
    //      A flag to create a DOM_XMLDecl node in the ODM tree if it exists
    //      This is an extension to xerces implementation
    //
    //   fGrammarPool
    //      The grammar pool passed from external application (through derivatives).
    //      which could be 0, not owned.
    //
    // -----------------------------------------------------------------------
    bool                    fToCreateXMLDeclTypeNode;
    bool                    fCreateEntityReferenceNodes;
    bool                    fIncludeIgnorableWhitespace;
    bool                    fParseInProgress;
    bool                    fWithinElement;
    DOM_Node                fCurrentParent;
    DOM_Node                fCurrentNode;
    DOM_Document            fDocument;
    EntityResolver*         fEntityResolver;
    XMLEntityResolver*      fXMLEntityResolver;
    ErrorHandler*           fErrorHandler;
    PSVIHandler*            fPSVIHandler;
    ValueStackOf<DOM_Node>* fNodeStack;
    XMLScanner*             fScanner;
    DocumentTypeImpl*       fDocumentType;
    GrammarResolver*        fGrammarResolver;
    XMLStringPool*          fURIStringPool;
    XMLValidator*           fValidator;
    MemoryManager*          fMemoryManager;
    XMLGrammarPool*         fGrammarPool;
};


// ---------------------------------------------------------------------------
//  DOMParser: Handlers for the XMLEntityHandler interface
// ---------------------------------------------------------------------------
inline void DOMParser::endInputSource(const InputSource&)
{
    // The DOM entity resolver doesn't handle this
}

inline bool DOMParser::expandSystemId(const XMLCh* const, XMLBuffer&)
{
    // The DOM entity resolver doesn't handle this
    return false;
}

inline void DOMParser::resetEntities()
{
    // Nothing to do on this one
}

inline void DOMParser::startInputSource(const InputSource&)
{
    // The DOM entity resolver doesn't handle this
}


// ---------------------------------------------------------------------------
//  DOMParser: Getter methods
// ---------------------------------------------------------------------------
inline DOM_Document DOMParser::getDocument()
{
    return fDocument;
}

inline ErrorHandler* DOMParser::getErrorHandler()
{
    return fErrorHandler;
}

inline const ErrorHandler* DOMParser::getErrorHandler() const
{
    return fErrorHandler;
}

inline PSVIHandler* DOMParser::getPSVIHandler()
{
    return fPSVIHandler;
}

inline const PSVIHandler* DOMParser::getPSVIHandler() const
{
    return fPSVIHandler;
}

inline EntityResolver* DOMParser::getEntityResolver()
{
    return fEntityResolver;
}

inline XMLEntityResolver* DOMParser::getXMLEntityResolver()
{
    return fXMLEntityResolver;
}

inline const XMLEntityResolver* DOMParser::getXMLEntityResolver() const
{
    return fXMLEntityResolver;
}

inline const EntityResolver* DOMParser::getEntityResolver() const
{
    return fEntityResolver;
}

inline bool DOMParser::getExpandEntityReferences() const
{
    return !fCreateEntityReferenceNodes;
}
inline bool DOMParser::getCreateEntityReferenceNodes() const
{
    return fCreateEntityReferenceNodes;
}

inline bool DOMParser::getIncludeIgnorableWhitespace() const
{
    return fIncludeIgnorableWhitespace;
}

inline const XMLScanner& DOMParser::getScanner() const
{
    return *fScanner;
}

inline bool DOMParser::getToCreateXMLDeclTypeNode() const
{
    return fToCreateXMLDeclTypeNode;
}


// ---------------------------------------------------------------------------
//  DOMParser: Setter methods
// ---------------------------------------------------------------------------
inline void DOMParser::setExpandEntityReferences(const bool expand)
{
    fCreateEntityReferenceNodes = !expand;
}

inline void DOMParser::setCreateEntityReferenceNodes(const bool create)
{
    fCreateEntityReferenceNodes = create;
}

inline void DOMParser::setIncludeIgnorableWhitespace(const bool include)
{
    fIncludeIgnorableWhitespace = include;
}

inline void DOMParser::setToCreateXMLDeclTypeNode(const bool create)
{
    fToCreateXMLDeclTypeNode = create;
}


// ---------------------------------------------------------------------------
//  DOMParser: Protected getter methods
// ---------------------------------------------------------------------------
inline DOM_Node DOMParser::getCurrentNode()
{
    return fCurrentNode;
}

// ---------------------------------------------------------------------------
//  DOMParser: Protected setter methods
// ---------------------------------------------------------------------------
inline void DOMParser::setCurrentNode(DOM_Node toSet)
{
    fCurrentNode = toSet;
}

inline void DOMParser::setDocument(DOM_Document toSet)
{
    fDocument = toSet;
}

XERCES_CPP_NAMESPACE_END

#endif
