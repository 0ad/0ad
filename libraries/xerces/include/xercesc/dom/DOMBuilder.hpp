#ifndef DOMBuilder_HEADER_GUARD_
#define DOMBuilder_HEADER_GUARD_

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
 * $Id: DOMBuilder.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 *
 */


#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN


class DOMErrorHandler;
class DOMEntityResolver;
class DOMInputSource;
class DOMBuilderFilter;
class DOMNode;
class DOMDocument;
class Grammar;

/**
 * DOMBuilder provides an API for parsing XML documents and building the
 * corresponding DOM document tree. A DOMBuilder instance is obtained from
 * the DOMImplementationLS interface by invoking its createDOMBuilder method.
 * This implementation also allows the applications to install an error and
 * an entity handler (useful extensions to the DOM specification).
 *
 * @since DOM Level 3
 *
 */
class CDOM_EXPORT DOMBuilder
{
protected :
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMBuilder() {};
    //@}

private:    
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMBuilder(const DOMBuilder &);
    DOMBuilder & operator = (const DOMBuilder &);
    //@}

public:
    // -----------------------------------------------------------------------
    //  All constructors are hidden, just the destructor is available
    // -----------------------------------------------------------------------
    /** @name Destructor */
    //@{
    /**
     * Destructor
     *
     */
    virtual ~DOMBuilder() {};
    //@}

    // -----------------------------------------------------------------------
    //  Class types
    // -----------------------------------------------------------------------
    /** @name Public Constants */
    //@{
    /**
     * Action types for use in parseWithContext.
     *
     * <p> <code>ACTION_REPLACE</code>:
     * Replace the context node with the result of parsing the input source.
     * For this action to work the context node must be an
     * <code>DOMElement</code>, <code>DOMText</code>, <code>DOMCDATASection</code>,
     * <code>DOMComment</code>, <code>DOMProcessingInstruction</code>, or
     * <code>DOMEntityReference</code> node.</p>
     *
     * <p> <code>ACTION_APPEND</code>:
     * Append the result of parsing the input source to the context node. For
     * this action to work, the context node must be an <code>DOMElement</code>.</p>
     *
     * <p> <code>ACTION_INSERT_AFTER</code>:
     * Insert the result of parsing the input source after the context node.
     * For this action to work the context nodes parent must be an
     * <code>DOMElement</code>.</p>
     *
     * <p> <code>ACTION_INSERT_BEFORE</code>:
     * Insert the result of parsing the input source before the context node.
     * For this action to work the context nodes parent must be an
     * <code>DOMElement</code>.</p>
     *
     * @see parseWithContext(...)
     * @since DOM Level 3
     */
    enum ActionType
    {
        ACTION_REPLACE            = 1,
        ACTION_APPEND_AS_CHILDREN = 2,
        ACTION_INSERT_AFTER       = 3,
        ACTION_INSERT_BEFORE      = 4
    };
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMBuilder interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------

    /**
      * Get a pointer to the error handler
      *
      * This method returns the installed error handler. If no handler
      * has been installed, then it will be a zero pointer.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @return The pointer to the installed error handler object.
      * @since DOM Level 3
      */
    virtual DOMErrorHandler* getErrorHandler() = 0;

    /**
      * Get a const pointer to the error handler
      *
      * This method returns the installed error handler.  If no handler
      * has been installed, then it will be a zero pointer.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @return A const pointer to the installed error handler object.
      * @since DOM Level 3
      */
    virtual const DOMErrorHandler* getErrorHandler() const = 0;

    /**
      * Get a pointer to the entity resolver
      *
      * This method returns the installed entity resolver.  If no resolver
      * has been installed, then it will be a zero pointer.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @return The pointer to the installed entity resolver object.
      * @since DOM Level 3
      */
    virtual DOMEntityResolver* getEntityResolver() = 0;

    /**
      * Get a const pointer to the entity resolver
      *
      * This method returns the installed entity resolver. If no resolver
      * has been installed, then it will be a zero pointer.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @return A const pointer to the installed entity resolver object.
      * @since DOM Level 3
      */
    virtual const DOMEntityResolver* getEntityResolver() const = 0;

    /**
      * Get a pointer to the application filter
      *
      * This method returns the installed application filter. If no filter
      * has been installed, then it will be a zero pointer.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @return The pointer to the installed application filter.
      * @since DOM Level 3
      */
    virtual DOMBuilderFilter* getFilter() = 0;

    /**
      * Get a const pointer to the application filter
      *
      * This method returns the installed application filter. If no filter
      * has been installed, then it will be a zero pointer.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @return A const pointer to the installed application filter
      * @since DOM Level 3
      */
    virtual const DOMBuilderFilter* getFilter() const = 0;

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    /**
      * Set the error handler
      *
      * This method allows applications to install their own error handler
      * to trap error and warning messages.
      *
      * <i>Any previously set handler is merely dropped, since the parser
      * does not own them.</i>
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @param handler  A const pointer to the user supplied error
      *                 handler.
      *
      * @see #getErrorHandler
      * @since DOM Level 3
      */
    virtual void setErrorHandler(DOMErrorHandler* const handler) = 0;

    /**
      * Set the entity resolver
      *
      * This method allows applications to install their own entity
      * resolver. By installing an entity resolver, the applications
      * can trap and potentially redirect references to external
      * entities.
      *
      * <i>Any previously set resolver is merely dropped, since the parser
      * does not own them.</i>
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @param handler  A const pointer to the user supplied entity
      *                 resolver.
      *
      * @see #getEntityResolver
      * @since DOM Level 3
      */
    virtual void setEntityResolver(DOMEntityResolver* const handler) = 0;

    /**
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
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @param filter  A const pointer to the user supplied application
      *                filter.
      *
      * @see #getFilter
      * @since DOM Level 3
      */
    virtual void setFilter(DOMBuilderFilter* const filter) = 0;

    // -----------------------------------------------------------------------
    //  Feature methods
    // -----------------------------------------------------------------------
    /**
      * Set the state of a feature
      *
      * It is possible for a DOMBuilder to recognize a feature name but to be
      * unable to set its value.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * See http://xml.apache.org/xerces-c/program-dom.html#DOMBuilderFeatures for
      * the list of supported features.
      *
      * @param name  The feature name.
      * @param state The requested state of the feature (true or false).
      * @exception DOMException
      *     NOT_SUPPORTED_ERR: Raised when the DOMBuilder recognizes the
      *     feature name but cannot set the requested value.
      *     <br>NOT_FOUND_ERR: Raised when the DOMBuilder does not recognize
      *     the feature name.
      *
      * @see #setFeature
      * @see #canSetFeature
      * @since DOM Level 3
      */
    virtual void setFeature(const XMLCh* const name, const bool state) = 0;

    /**
      * Look up the value of a feature.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @param name The feature name.
      * @return The current state of the feature (true or false)
      * @exception DOMException
      *     NOT_FOUND_ERR: Raised when the DOMBuilder does not recognize
      *     the feature name.
      *
      * @see #getFeature
      * @see #canSetFeature
      * @since DOM Level 3
      */
    virtual bool getFeature(const XMLCh* const name) const = 0;

    /**
      * Query whether setting a feature to a specific value is supported.
      *
      * <p><b>"Experimental - subject to change"</b></p>
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
      * @since DOM Level 3
      */
    virtual bool canSetFeature(const XMLCh* const name, const bool state) const = 0;

    // -----------------------------------------------------------------------
    //  Parsing methods
    // -----------------------------------------------------------------------
    /**
      * Parse via an input source object
      *
      * This method invokes the parsing process on the XML file specified
      * by the DOMInputSource parameter. This API is borrowed from the
      * SAX Parser interface.
      *
      * The parser owns the returned DOMDocument.  It will be deleted
      * when the parser is released.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @param source A const reference to the DOMInputSource object which
      *               points to the XML file to be parsed.
      * @return If the DOMBuilder is a synchronous DOMBuilder the newly created
      *         and populated DOMDocument is returned. If the DOMBuilder is
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
      * @see resetDocumentPool
      * @since DOM Level 3
      */
    virtual DOMDocument* parse(const DOMInputSource& source) = 0;

    /**
      * Parse via a file path or URL
      *
      * This method invokes the parsing process on the XML file specified by
      * the Unicode string parameter 'systemId'.
      *
      * The parser owns the returned DOMDocument.  It will be deleted
      * when the parser is released.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @param systemId A const XMLCh pointer to the Unicode string which
      *                 contains the path to the XML file to be parsed.
      * @return If the DOMBuilder is a synchronous DOMBuilder the newly created
      *         and populated DOMDocument is returned. If the DOMBuilder is
      *         asynchronous then <code>null</code> is returned since the
      *         document object is not yet parsed when this method returns.
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOMException A DOM exception as per DOM spec.
      *
      * @see #parse(DOMInputSource,...)
      * @see resetDocumentPool
      * @since DOM Level 3
      */
    virtual DOMDocument* parseURI(const XMLCh* const systemId) = 0;

    /**
      * Parse via a file path or URL (in the local code page)
      *
      * This method invokes the parsing process on the XML file specified by
      * the native char* string parameter 'systemId'.
      *
      * The parser owns the returned DOMDocument.  It will be deleted
      * when the parser is released.
      *
      * <p><b>"Experimental - subject to change"</b></p>
      *
      * @param systemId A const char pointer to a native string which
      *                 contains the path to the XML file to be parsed.
      * @return If the DOMBuilder is a synchronous DOMBuilder the newly created
      *         and populated DOMDocument is returned. If the DOMBuilder is
      *         asynchronous then <code>null</code> is returned since the
      *         document object is not yet parsed when this method returns.
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOMException A DOM exception as per DOM spec.
      *
      * @see #parse(DOMInputSource,...)
      * @see resetDocumentPool
      */
    virtual DOMDocument* parseURI(const char* const systemId) = 0;

    /**
      * Parse via an input source object
      *
      * This method invokes the parsing process on the XML file specified
      * by the DOMInputSource parameter, and inserts the content into an
      * existing document at the position specified with the contextNode
      * and action arguments. When parsing the input stream the context node
      * is used for resolving unbound namespace prefixes.
      *
      * <p><b>"Experimental - subject to change"</b></p>
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
      * @since DOM Level 3
      */
    virtual void parseWithContext
    (
        const   DOMInputSource& source
        ,       DOMNode* const contextNode
        , const short action
    ) = 0;
    //@}

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
    virtual void* getProperty(const XMLCh* const name) const = 0 ;

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
    virtual void setProperty(const XMLCh* const name, void* value) = 0 ;

    /**
     * Called to indicate that this DOMBuilder is no longer in use
     * and that the implementation may relinquish any resources associated with it.
     *
     * Access to a released object will lead to unexpected result.
     */
    virtual void              release() = 0;

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
    virtual void              resetDocumentPool() = 0;

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
                                 const bool toCache = false) = 0;

    /**
      * Preparse schema grammar (XML Schema, DTD, etc.) via a file path or URL
      *
      * This method invokes the preparsing process on a schema grammar XML
      * file specified by the file path parameter. If the 'toCache' flag is
      * enabled, the parser will cache the grammars for re-use. If a grammar
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
                                 const bool toCache = false) = 0;

    /**
      * Preparse schema grammar (XML Schema, DTD, etc.) via a file path or URL
      *
      * This method invokes the preparsing process on a schema grammar XML
      * file specified by the file path parameter. If the 'toCache' flag is
      * enabled, the parser will cache the grammars for re-use. If a grammar
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
      *
      * @exception SAXException Any SAX exception, possibly
      *            wrapping another exception.
      * @exception XMLException An exception from the parser or client
      *            handler code.
      * @exception DOMException A DOM exception as per DOM spec.
      */
    virtual Grammar* loadGrammar(const char* const systemId,
                                 const short grammarType,
                                 const bool toCache = false) = 0;

    /**
     * Retrieve the grammar that is associated with the specified namespace key
     *
     * @param  nameSpaceKey Namespace key
     * @return Grammar associated with the Namespace key.
     */
    virtual Grammar* getGrammar(const XMLCh* const nameSpaceKey) const = 0;

    /**
     * Retrieve the grammar where the root element is declared.
     *
     * @return Grammar where root element declared
     */
    virtual Grammar* getRootGrammar() const = 0;

    /**
     * Returns the string corresponding to a URI id from the URI string pool.
     *
     * @param uriId id of the string in the URI string pool.
     * @return URI string corresponding to the URI id.
     */
    virtual const XMLCh* getURIText(unsigned int uriId) const = 0;

    /**
      * Clear the cached grammar pool
      */
    virtual void resetCachedGrammarPool() = 0;

    /**
      * Returns the current src offset within the input source.
      *
      * @return offset within the input source
      */
    virtual unsigned int getSrcOffset() const = 0;

    //@}

};


XERCES_CPP_NAMESPACE_END

#endif
