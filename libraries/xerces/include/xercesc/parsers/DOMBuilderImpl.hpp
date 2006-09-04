/*
 * Copyright 2002,2004 The Apache Software Foundation.
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
 * $Id: DOMBuilderImpl.hpp 191708 2005-06-21 19:02:15Z cargilld $
 *
 */

#if !defined(DOMBUILDERIMPL_HPP)
#define DOMBUILDERIMPL_HPP


#include <xercesc/parsers/AbstractDOMParser.hpp>
#include <xercesc/dom/DOMBuilder.hpp>
#include <xercesc/util/XercesDefs.hpp>


XERCES_CPP_NAMESPACE_BEGIN

class XMLEntityResolver;
class XMLResourceIdentifier;

 /**
  * Introduced in DOM Level 3
  *
  * DOMBuilderImpl provides an implementation of a DOMBuilder interface.
  * A DOMBuilder instance is obtained from the DOMImplementationLS interface
  * by invoking its createDOMBuilder method.
  */
class PARSERS_EXPORT DOMBuilderImpl : public AbstractDOMParser,
                                      public DOMBuilder
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Detructor
    // -----------------------------------------------------------------------

    /** @name Constructors and Destructor */
    //@{
    /** Construct a DOMBuilderImpl, with an optional validator
      *
      * Constructor with an instance of validator class to use for
      * validation. If you don't provide a validator, a default one will
      * be created for you in the scanner.
      *
      * @param gramPool   Pointer to the grammar pool instance from 
      *                   external application.
      *                   The parser does NOT own it.
      *
      * @param valToAdopt Pointer to the validator instance to use. The
      *                   parser is responsible for freeing the memory.
      */
    DOMBuilderImpl
    (
          XMLValidator* const   valToAdopt = 0
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
        , XMLGrammarPool* const gramPool = 0        
    );

    /**
      * Destructor
      */
    virtual ~DOMBuilderImpl();

    //@}

    // -----------------------------------------------------------------------
    //  Implementation of DOMBuilder interface
    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------

    /** @name Getter methods */
    //@{

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Get a pointer to the error handler
      *
      * This method returns the installed error handler. If no handler
      * has been installed, then it will be a zero pointer.
      *
      * @return The pointer to the installed error handler object.
      */
    DOMErrorHandler* getErrorHandler();

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Get a const pointer to the error handler
      *
      * This method returns the installed error handler.  If no handler
      * has been installed, then it will be a zero pointer.
      *
      * @return A const pointer to the installed error handler object.
      */
    const DOMErrorHandler* getErrorHandler() const;

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Get a pointer to the entity resolver
      *
      * This method returns the installed entity resolver.  If no resolver
      * has been installed, then it will be a zero pointer.
      *
      * @return The pointer to the installed entity resolver object.
      */
    DOMEntityResolver* getEntityResolver();

    /**
      * Get a pointer to the entity resolver
      *
      * This method returns the installed entity resolver.  If no resolver
      * has been installed, then it will be a zero pointer.
      *
      * @return The pointer to the installed entity resolver object.
      */
    XMLEntityResolver* getXMLEntityResolver();
 
    /**
      * Get a const pointer to the entity resolver
      *
      * This method returns the installed entity resolver. If no resolver
      * has been installed, then it will be a zero pointer.
      *
      * @return A const pointer to the installed entity resolver object.
      */
    const XMLEntityResolver* getXMLEntityResolver() const;

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Get a const pointer to the entity resolver
      *
      * This method returns the installed entity resolver. If no resolver
      * has been installed, then it will be a zero pointer.
      *
      * @return A const pointer to the installed entity resolver object.
      */
    const DOMEntityResolver* getEntityResolver() const;

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Get a pointer to the application filter
      *
      * This method returns the installed application filter. If no filter
      * has been installed, then it will be a zero pointer.
      *
      * @return The pointer to the installed application filter.
      */
    DOMBuilderFilter* getFilter();

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Get a const pointer to the application filter
      *
      * This method returns the installed application filter. If no filter
      * has been installed, then it will be a zero pointer.
      *
      * @return A const pointer to the installed application filter
      */
    const DOMBuilderFilter* getFilter() const;

    //@}


    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------

    /** @name Setter methods */
    //@{

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Set the error handler
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
    void setErrorHandler(DOMErrorHandler* const handler);

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Set the entity resolver
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
    void setEntityResolver(DOMEntityResolver* const handler);

    /**
      * Set the entity resolver
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

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Set the application filter
      *
      * When the application provides a filter, the parser will call out to
      * the filter at the completion of the construction of each Element node.
      * The filter implementation can choose to remove the element from the
      * document being constructed (unless the element is the document element)
      * or to terminate the parse early. If the document is being validated
      * when it's loaded the validation happens before the filter is called.
      *
      * <i>Any previously set filter is merely dropped, since the parser
      * does not own them.</i>
      *
      * @param filter  A const pointer to the user supplied application
      *                filter.
      *
      * @see #getFilter
      */
    void setFilter(DOMBuilderFilter* const filter);

    //@}


    // -----------------------------------------------------------------------
    //  Feature methods
    // -----------------------------------------------------------------------
    /** @name Feature methods */
    //@{

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Set the state of a feature
      *
      * It is possible for a DOMBuilder to recognize a feature name but to be
      * unable to set its value.
      *
      * @param name  The feature name.
      * @param state The requested state of the feature (true or false).
      * @exception DOMException
      *     NOT_SUPPORTED_ERR: Raised when the DOMBuilder recognizes the
      *     feature name but cannot set the requested value.
      *     <br>NOT_FOUND_ERR: Raised when the DOMBuilder does not recognize
      *     the feature name.
      *
      * @see #getFeature
      * @see #canSetFeature
      */
    void setFeature(const XMLCh* const name, const bool state);

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Look up the value of a feature.
      *
      * @param name The feature name.
      * @return The current state of the feature (true or false)
      * @exception DOMException
      *     NOT_FOUND_ERR: Raised when the DOMBuilder does not recognize
      *     the feature name.
      *
      * @see #setFeature
      * @see #canSetFeature
      */
    bool getFeature(const XMLCh* const name) const;

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Query whether setting a feature to a specific value is supported.
      *
      * @param name  The feature name.
      * @param state The requested state of the feature (true or false).
      * @return <code>true</code> if the feature could be successfully set
      *     to the specified value, or <code>false</code> if the feature
      *     is not recognized or the requested value is not supported. The
      *     value of the feature itself is not changed.
      *
      * @see #getFeature
      * @see #setFeature
      */
    bool canSetFeature(const XMLCh* const name, const bool state) const;

    //@}

    // -----------------------------------------------------------------------
    //  Parsing methods
    // -----------------------------------------------------------------------
    /** @name Parsing methods */
    //@{

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Parse via an input source object
      *
      * This method invokes the parsing process on the XML file specified
      * by the DOMInputSource parameter. This API is borrowed from the
      * SAX Parser interface.
      *
      * @param source A const reference to the DOMInputSource object which
      *               points to the XML file to be parsed.
      * @return If the DOMBuilder is a synchronous DOMBuilder the newly created
      *         and populated Document is returned. If the DOMBuilder is
      *         asynchronous then <code>null</code> is returned since the
      *         document object is not yet parsed when this method returns.
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOMException A DOM exception as per DOM spec.
      *
      * @see DOMInputSource#DOMInputSource
      * @see #setEntityResolver
      * @see #setErrorHandler
      */
    DOMDocument* parse(const DOMInputSource& source);

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Parse via a file path or URL
      *
      * This method invokes the parsing process on the XML file specified by
      * the Unicode string parameter 'systemId'.
      *
      * @param systemId A const XMLCh pointer to the Unicode string which
      *                 contains the path to the XML file to be parsed.
      * @return If the DOMBuilder is a synchronous DOMBuilder the newly created
      *         and populated Document is returned. If the DOMBuilder is
      *         asynchronous then <code>null</code> is returned since the
      *         document object is not yet parsed when this method returns.
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOM_DOMException A DOM exception as per DOM spec.
      *
      * @see #parse(DOMInputSource,...)
      */
    DOMDocument* parseURI(const XMLCh* const systemId);

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Parse via a file path or URL (in the local code page)
      *
      * This method invokes the parsing process on the XML file specified by
      * the native char* string parameter 'systemId'.
      *
      * @param systemId A const char pointer to a native string which
      *                 contains the path to the XML file to be parsed.
      * @return If the DOMBuilder is a synchronous DOMBuilder the newly created
      *         and populated Document is returned. If the DOMBuilder is
      *         asynchronous then <code>null</code> is returned since the
      *         document object is not yet parsed when this method returns.
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOM_DOMException A DOM exception as per DOM spec.
      *
      * @see #parse(DOMInputSource,...)
      */
    DOMDocument* parseURI(const char* const systemId);

    /**
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * Parse via an input source object
      *
      * This method invokes the parsing process on the XML file specified
      * by the DOMInputSource parameter, and inserts the content into an
      * existing document at the position specified with the contextNode
      * and action arguments. When parsing the input stream the context node
      * is used for resolving unbound namespace prefixes.
      *
      * @param source A const reference to the DOMInputSource object which
      *               points to the XML file to be parsed.
      * @param contextNode The node that is used as the context for the data
      *                    that is being parsed. This node must be a Document
      *                    node, a DocumentFragment node, or a node of a type
      *                    that is allowed as a child of an element, e.g. it
      *                    can not be an attribute node.
      * @param action This parameter describes which action should be taken
      *               between the new set of node being inserted and the
      *               existing children of the context node.
      * @exception DOMException
      *     NOT_SUPPORTED_ERR: Raised when the DOMBuilder doesn't support
      *     this method.
      *     <br>NO_MODIFICATION_ALLOWED_ERR: Raised if the context node is
      *     readonly.
      */
    virtual void parseWithContext
    (
        const   DOMInputSource& source
        ,       DOMNode* const contextNode
        , const short action
    );


    // -----------------------------------------------------------------------
    //  Non-standard Extension
    // -----------------------------------------------------------------------
    /** @name Non-standard Extension */
    //@{

    /**
      * Query the current value of a property in a DOMBuilder.
      *
      * The builder owns the returned pointer.  The memory allocated for
      * the returned pointer will be destroyed when the builder is deleted.
      *
      * To ensure assessiblity of the returned information after the builder
      * is deleted, callers need to copy and store the returned information
      * somewhere else; otherwise you may get unexpected result.  Since the returned
      * pointer is a generic void pointer, see
      * http://xml.apache.org/xerces-c/program-dom.html#DOMBuilderProperties to learn
      * exactly what type of property value each property returns for replication.
      *
      * @param name The unique identifier (URI) of the property being set.
      * @return     The current value of the property.  The pointer spans the same
      *             life-time as the parser.  A null pointer is returned if nothing
      *             was specified externally.
      * @exception DOMException
      *     <br>NOT_FOUND_ERR: Raised when the DOMBuilder does not recognize
      *     the requested property.
      */
    virtual void* getProperty(const XMLCh* const name) const;

    /**
      * Set the value of any property in a DOMBuilder.
      * See http://xml.apache.org/xerces-c/program-dom.html#DOMBuilderProperties for
      * the list of supported properties.
      *
      * It takes a void pointer as the property value.  Application is required to initialize this void
      * pointer to a correct type.  See http://xml.apache.org/xerces-c/program-dom.html#DOMBuilderProperties
      * to learn exactly what type of property value each property expects for processing.
      * Passing a void pointer that was initialized with a wrong type will lead to unexpected result.
      * If the same property is set more than once, the last one takes effect.
      *
      * @param name The unique identifier (URI) of the property being set.
      * @param value The requested value for the property.
      *            See http://xml.apache.org/xerces-c/program-dom.html#DOMBuilderProperties to learn
      *            exactly what type of property value each property expects for processing.
      *            Passing a void pointer that was initialized with a wrong type will lead
      *            to unexpected result.
      * @exception DOMException
      *     <br>NOT_FOUND_ERR: Raised when the DOMBuilder does not recognize
      *     the requested property.
      */
    virtual void setProperty(const XMLCh* const name, void* value);

    /**
     * Called to indicate that this DOMBuilder is no longer in use
     * and that the implementation may relinquish any resources associated with it.
     *
     */
    virtual void              release();

    /** Reset the documents vector pool and release all the associated memory
      * back to the system.
      *
      * When parsing a document using a DOM parser, all memory allocated
      * for a DOM tree is associated to the DOM document.
      *
      * If you do multiple parse using the same DOM parser instance, then
      * multiple DOM documents will be generated and saved in a vector pool.
      * All these documents (and thus all the allocated memory)
      * won't be deleted until the parser instance is destroyed.
      *
      * If you don't need these DOM documents anymore and don't want to
      * destroy the DOM parser instance at this moment, then you can call this method
      * to reset the document vector pool and release all the allocated memory
      * back to the system.
      *
      * It is an error to call this method if you are in the middle of a
      * parse (e.g. in the mid of a progressive parse).
      *
      * @exception IOException An exception from the parser if this function
      *            is called when a parse is in progress.
      *
      */
    virtual void resetDocumentPool();

    /**
      * Preparse schema grammar (XML Schema, DTD, etc.) via an input source
      * object.
      *
      * This method invokes the preparsing process on a schema grammar XML
      * file specified by the DOMInputSource parameter. If the 'toCache' flag
      * is enabled, the parser will cache the grammars for re-use. If a grammar
      * key is found in the pool, no caching of any grammar will take place.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @param source A const reference to the DOMInputSource object which
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
      * @see DOMInputSource#DOMInputSource
      */
    virtual Grammar* loadGrammar(const DOMInputSource& source,
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
    virtual Grammar* loadGrammar(const XMLCh* const systemId,
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
    virtual Grammar* loadGrammar(const char* const systemId,
                                 const short grammarType,
                                 const bool toCache = false);

    /**
     * Retrieve the grammar that is associated with the specified namespace key
     *
     * @param  nameSpaceKey Namespace key
     * @return Grammar associated with the Namespace key.
     */
    virtual Grammar* getGrammar(const XMLCh* const nameSpaceKey) const;

    /**
     * Retrieve the grammar where the root element is declared.
     *
     * @return Grammar where root element declared
     */
    virtual Grammar* getRootGrammar() const;

    /**
     * Returns the string corresponding to a URI id from the URI string pool.
     *
     * @param uriId id of the string in the URI string pool.
     * @return URI string corresponding to the URI id.
     */
    virtual const XMLCh* getURIText(unsigned int uriId) const;

    /**
      * Clear the cached grammar pool
      */
    virtual void resetCachedGrammarPool();

    /**
      * Returns the current src offset within the input source.
      * To be used only while parsing is in progress.
      *
      * @return offset within the input source
      */
    virtual unsigned int getSrcOffset() const;

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
      * @see DOMErrorHandler
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
      *         The returned InputSource is owned by the DOMBuilder which is
      *         responsible to clean up the memory.
      * @see DOMEntityResolver
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


private :
    // -----------------------------------------------------------------------
    //  Initialize/Cleanup methods
    // -----------------------------------------------------------------------
    void resetParse();

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fEntityResolver
    //      The installed DOM entity resolver, if any. Null if none.
    //
    //  fErrorHandler
    //      The installed DOM error handler, if any. Null if none.
    //
    //  fFilter
    //      The installed application filter, if any. Null if none.
    //
    //  fCharsetOverridesXMLEncoding
    //      Indicates if the "charset-overrides-xml-encoding" is set or not
    //
    //  fUserAdoptsDocument
    //      The DOMDocument ownership has been transferred to application
    //      If set to true, the parser does not own the document anymore
    //      and thus will not release its memory.
    //-----------------------------------------------------------------------
    bool                        fAutoValidation;
    bool                        fValidation;
    DOMEntityResolver*          fEntityResolver;
    XMLEntityResolver*          fXMLEntityResolver;
    DOMErrorHandler*            fErrorHandler;
    DOMBuilderFilter*           fFilter;
    bool                        fCharsetOverridesXMLEncoding;
    bool                        fUserAdoptsDocument;

    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    DOMBuilderImpl(const DOMBuilderImpl &);
    DOMBuilderImpl & operator = (const DOMBuilderImpl &);
};



// ---------------------------------------------------------------------------
//  DOMBuilderImpl: Handlers for the XMLEntityHandler interface
// ---------------------------------------------------------------------------
inline void DOMBuilderImpl::endInputSource(const InputSource&)
{
    // The DOM entity resolver doesn't handle this
}

inline bool DOMBuilderImpl::expandSystemId(const XMLCh* const, XMLBuffer&)
{
    // The DOM entity resolver doesn't handle this
    return false;
}

inline void DOMBuilderImpl::resetEntities()
{
    // Nothing to do on this one
}

inline void DOMBuilderImpl::startInputSource(const InputSource&)
{
    // The DOM entity resolver doesn't handle this
}


// ---------------------------------------------------------------------------
//  DOMBuilderImpl: Getter methods
// ---------------------------------------------------------------------------
inline DOMErrorHandler* DOMBuilderImpl::getErrorHandler()
{
    return fErrorHandler;
}

inline const DOMErrorHandler* DOMBuilderImpl::getErrorHandler() const
{
    return fErrorHandler;
}

inline DOMEntityResolver* DOMBuilderImpl::getEntityResolver()
{
    return fEntityResolver;
}

inline const DOMEntityResolver* DOMBuilderImpl::getEntityResolver() const
{
    return fEntityResolver;
}

inline XMLEntityResolver* DOMBuilderImpl::getXMLEntityResolver()
{
    return fXMLEntityResolver;
}

inline const XMLEntityResolver* DOMBuilderImpl::getXMLEntityResolver() const
{
    return fXMLEntityResolver;
}

inline DOMBuilderFilter* DOMBuilderImpl::getFilter()
{
    return fFilter;
}

inline const DOMBuilderFilter* DOMBuilderImpl::getFilter() const
{
    return fFilter;
}


XERCES_CPP_NAMESPACE_END

#endif
