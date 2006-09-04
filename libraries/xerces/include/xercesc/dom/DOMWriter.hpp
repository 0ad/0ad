#ifndef DOMWriter_HEADER_GUARD_
#define DOMWriter_HEADER_GUARD_

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
 * $Id: DOMWriter.hpp 231323 2005-08-10 20:58:40Z cargilld $
 */

/**
 *
 * DOMWriter provides an API for serializing (writing) a DOM document out in
 * an XML document. The XML data is written to an output stream, the type of
 * which depends on the specific language bindings in use. During
 * serialization of XML data, namespace fixup is done when possible.
 * <p> <code>DOMWriter</code> accepts any node type for serialization. For
 * nodes of type <code>Document</code> or <code>Entity</code>, well formed
 * XML will be created if possible. The serialized output for these node
 * types is either as a Document or an External Entity, respectively, and is
 * acceptable input for an XML parser. For all other types of nodes the
 * serialized form is not specified, but should be something useful to a
 * human for debugging or diagnostic purposes. Note: rigorously designing an
 * external (source) form for stand-alone node types that don't already have
 * one defined in  seems a bit much to take on here.
 * <p>Within a Document or Entity being serialized, Nodes are processed as
 * follows Documents are written including an XML declaration and a DTD
 * subset, if one exists in the DOM. Writing a document node serializes the
 * entire document.  Entity nodes, when written directly by
 * <code>writeNode</code> defined in the <code>DOMWriter</code> interface,
 * output the entity expansion but no namespace fixup is done. The resulting
 * output will be valid as an external entity.  Entity References nodes are
 * serializes as an entity reference of the form
 * <code>"&amp;entityName;"</code>) in the output. Child nodes (the
 * expansion) of the entity reference are ignored.  CDATA sections
 * containing content characters that can not be represented in the
 * specified output encoding are handled according to the
 * "split-cdata-sections" feature.If the feature is <code>true</code>, CDATA
 * sections are split, and the unrepresentable characters are serialized as
 * numeric character references in ordinary content. The exact position and
 * number of splits is not specified. If the feature is <code>false</code>,
 * unrepresentable characters in a CDATA section are reported as errors. The
 * error is not recoverable - there is no mechanism for supplying
 * alternative characters and continuing with the serialization. All other
 * node types (DOMElement, DOMText, etc.) are serialized to their corresponding
 * XML source form.
 * <p> Within the character data of a document (outside of markup), any
 * characters that cannot be represented directly are replaced with
 * character references. Occurrences of '&lt;' and '&amp;' are replaced by
 * the predefined entities &amp;lt; and &amp;amp. The other predefined
 * entities (&amp;gt, &amp;apos, etc.) are not used; these characters can be
 * included directly. Any character that can not be represented directly in
 * the output character encoding is serialized as a numeric character
 * reference.
 * <p> Attributes not containing quotes are serialized in quotes. Attributes
 * containing quotes but no apostrophes are serialized in apostrophes
 * (single quotes). Attributes containing both forms of quotes are
 * serialized in quotes, with quotes within the value represented by the
 * predefined entity &amp;quot;. Any character that can not be represented
 * directly in the output character encoding is serialized as a numeric
 * character reference.
 * <p> Within markup, but outside of attributes, any occurrence of a character
 * that cannot be represented in the output character encoding is reported
 * as an error. An example would be serializing the element
 * &lt;LaCa�ada/&gt; with the encoding="us-ascii".
 * <p> When requested by setting the <code>normalize-characters</code> feature
 * on <code>DOMWriter</code>, all data to be serialized, both markup and
 * character data, is W3C Text normalized according to the rules defined in
 * . The W3C Text normalization process affects only the data as it is being
 * written; it does not alter the DOM's view of the document after
 * serialization has completed.
 * <p>Namespaces are fixed up during serialization, the serialization process
 * will verify that namespace declarations, namespace prefixes and the
 * namespace URIs associated with Elements and Attributes are consistent. If
 * inconsistencies are found, the serialized form of the document will be
 * altered to remove them. The algorithm used for doing the namespace fixup
 * while seralizing a document is a combination of the algorithms used for
 * lookupNamespaceURI and lookupNamespacePrefix . previous paragraph to be
 * defined closer here.
 * <p>Any changes made affect only the namespace prefixes and declarations
 * appearing in the serialized data. The DOM's view of the document is not
 * altered by the serialization operation, and does not reflect any changes
 * made to namespace declarations or prefixes in the serialized output.
 * <p> While serializing a document the serializer will write out
 * non-specified values (such as attributes whose <code>specified</code> is
 * <code>false</code>) if the <code>output-default-values</code> feature is
 * set to <code>true</code>. If the <code>output-default-values</code> flag
 * is set to <code>false</code> and the <code>use-abstract-schema</code>
 * feature is set to <code>true</code> the abstract schema will be used to
 * determine if a value is specified or not, if
 * <code>use-abstract-schema</code> is not set the <code>specified</code>
 * flag on attribute nodes is used to determine if attribute values should
 * be written out.
 * <p> Ref to Core spec (1.1.9, XML namespaces, 5th paragraph) entity ref
 * description about warning about unbound entity refs. Entity refs are
 * always serialized as &amp;foo;, also mention this in the load part of
 * this spec.
 * <p> When serializing a document the DOMWriter checks to see if the document
 * element in the document is a DOM Level 1 element or a DOM Level 2 (or
 * higher) element (this check is done by looking at the localName of the
 * root element). If the root element is a DOM Level 1 element then the
 * DOMWriter will issue an error if a DOM Level 2 (or higher) element is
 * found while serializing. Likewise if the document element is a DOM Level
 * 2 (or higher) element and the DOMWriter sees a DOM Level 1 element an
 * error is issued. Mixing DOM Level 1 elements with DOM Level 2 (or higher)
 * is not supported.
 * <p> <code>DOMWriter</code>s have a number of named features that can be
 * queried or set. The name of <code>DOMWriter</code> features must be valid
 * XML names. Implementation specific features (extensions) should choose an
 * implementation dependent prefix to avoid name collisions.
 * <p>Here is a list of properties that must be recognized by all
 * implementations.
 * <dl>
 * <dt><code>"normalize-characters"</code></dt>
 * <dd>
 * <dl>
 * <dt><code>true</code></dt>
 * <dd>[
 * optional] (default) Perform the W3C Text Normalization of the characters
 * in document as they are written out. Only the characters being written
 * are (potentially) altered. The DOM document itself is unchanged. </dd>
 * <dt>
 * <code>false</code></dt>
 * <dd>[required] do not perform character normalization. </dd>
 * </dl></dd>
 * <dt>
 * <code>"split-cdata-sections"</code></dt>
 * <dd>
 * <dl>
 * <dt><code>true</code></dt>
 * <dd>[required] (default)
 * Split CDATA sections containing the CDATA section termination marker
 * ']]&gt;' or characters that can not be represented in the output
 * encoding, and output the characters using numeric character references.
 * If a CDATA section is split a warning is issued. </dd>
 * <dt><code>false</code></dt>
 * <dd>[
 * required] Signal an error if a <code>CDATASection</code> contains an
 * unrepresentable character. </dd>
 * </dl></dd>
 * <dt><code>"validation"</code></dt>
 * <dd>
 * <dl>
 * <dt><code>true</code></dt>
 * <dd>[
 * optional] Use the abstract schema to validate the document as it is being
 * serialized. If validation errors are found the error handler is notified
 * about the error. Setting this state will also set the feature
 * <code>use-abstract-schema</code> to <code>true</code>. </dd>
 * <dt><code>false</code></dt>
 * <dd>[
 * required] (default) Don't validate the document as it is being
 * serialized. </dd>
 * </dl></dd>
 * <dt><code>"expand-entity-references"</code></dt>
 * <dd>
 * <dl>
 * <dt><code>true</code></dt>
 * <dd>[
 * optional] Expand <code>EntityReference</code> nodes when serializing. </dd>
 * <dt>
 * <code>false</code></dt>
 * <dd>[required] (default) Serialize all
 * <code>EntityReference</code> nodes as XML entity references. </dd>
 * </dl></dd>
 * <dt>
 * <code>"whitespace-in-element-content"</code></dt>
 * <dd>
 * <dl>
 * <dt><code>true</code></dt>
 * <dd>[required] (
 * default) Output all white spaces in the document. </dd>
 * <dt><code>false</code></dt>
 * <dd>[
 * optional] Only output white space that is not within element content. The
 * implementation is expected to use the
 * <code>isWhitespaceInElementContent</code> flag on <code>Text</code> nodes
 * to determine if a text node should be written out or not. </dd>
 * </dl></dd>
 * <dt>
 * <code>"discard-default-content"</code></dt>
 * <dd>
 * <dl>
 * <dt><code>true</code></dt>
 * <dd>[required] (default
 * ) Use whatever information available to the implementation (i.e. XML
 * schema, DTD, the <code>specified</code> flag on <code>Attr</code> nodes,
 * and so on) to decide what attributes and content should be serialized or
 * not. Note that the <code>specified</code> flag on <code>Attr</code> nodes
 * in itself is not always reliable, it is only reliable when it is set to
 * <code>false</code> since the only case where it can be set to
 * <code>false</code> is if the attribute was created by a Level 1
 * implementation. </dd>
 * <dt><code>false</code></dt>
 * <dd>[required] Output all attributes and
 * all content. </dd>
 * </dl></dd>
 * <dt><code>"format-canonical"</code></dt>
 * <dd>
 * <dl>
 * <dt><code>true</code></dt>
 * <dd>[optional]
 * This formatting writes the document according to the rules specified in .
 * Setting this feature to true will set the feature "format-pretty-print"
 * to false. </dd>
 * <dt><code>false</code></dt>
 * <dd>[required] (default) Don't canonicalize the
 * output. </dd>
 * </dl></dd>
 * <dt><code>"format-pretty-print"</code></dt>
 * <dd>
 * <dl>
 * <dt><code>true</code></dt>
 * <dd>[optional]
 * Formatting the output by adding whitespace to produce a pretty-printed,
 * indented, human-readable form. The exact form of the transformations is
 * not specified by this specification. Setting this feature to true will
 * set the feature "format-canonical" to false. </dd>
 * <dt><code>false</code></dt>
 * <dd>[required]
 * (default) Don't pretty-print the result. </dd>
 * </dl></dd>
 * </dl>
 * <p>See also the <a href='http://www.w3.org/TR/2002/WD-DOM-Level-3-ASLS-20020409'>Document Object Model (DOM) Level 3 Abstract Schemas and Load
 * and Save Specification</a>.
 *
 * @since DOM Level 3
 */


#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMWriterFilter.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/framework/XMLFormatter.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class CDOM_EXPORT DOMWriter {
protected :
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMWriter() {};
    //@}
private:        
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMWriter(const DOMWriter &);
    DOMWriter & operator = (const DOMWriter &);
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
    virtual ~DOMWriter() {};
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMWriter interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{
    // -----------------------------------------------------------------------
    //  Feature methods
    // -----------------------------------------------------------------------
    /**
     * Query whether setting a feature to a specific value is supported.
     * <br>The feature name has the same form as a DOM hasFeature string.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @param featName The feature name, which is a DOM has-feature style string.
     * @param state The requested state of the feature (<code>true</code> or
     *   <code>false</code>).
     * @return <code>true</code> if the feature could be successfully set to
     *   the specified value, or <code>false</code> if the feature is not
     *   recognized or the requested value is not supported. The value of
     *   the feature itself is not changed.
     * @since DOM Level 3
     */
    virtual bool           canSetFeature(const XMLCh* const featName
                                       , bool               state) const = 0;
    /**
     * Set the state of a feature.
     * <br>The feature name has the same form as a DOM hasFeature string.
     * <br>It is possible for a <code>DOMWriter</code> to recognize a feature
     * name but to be unable to set its value.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @param featName The feature name.
     * @param state The requested state of the feature (<code>true</code> or
     *   <code>false</code>).
     * @exception DOMException
     *   Raise a NOT_SUPPORTED_ERR exception when the <code>DOMWriter</code>
     *   recognizes the feature name but cannot set the requested value.
     *   <br>Raise a NOT_FOUND_ERR When the <code>DOMWriter</code> does not
     *   recognize the feature name.
     * @see   getFeature
     * @since DOM Level 3
     */
    virtual void            setFeature(const XMLCh* const featName
                                     , bool               state) = 0;

    /**
     * Look up the value of a feature.
     * <br>The feature name has the same form as a DOM hasFeature string
     * @param featName The feature name, which is a string with DOM has-feature
     *   syntax.
     * @return The current state of the feature (<code>true</code> or
     *   <code>false</code>).
     * @exception DOMException
     *   Raise a NOT_FOUND_ERR When the <code>DOMWriter</code> does not
     *   recognize the feature name.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @see   setFeature
     * @since DOM Level 3
     */
    virtual bool               getFeature(const XMLCh* const featName) const = 0;

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    /**
     * The character encoding in which the output will be written.
     * <br> The encoding to use when writing is determined as follows: If the
     * encoding attribute has been set, that value will be used.If the
     * encoding attribute is <code>null</code> or empty, but the item to be
     * written includes an encoding declaration, that value will be used.If
     * neither of the above provides an encoding name, a default encoding of
     * "UTF-8" will be used.
     * <br>The default value is <code>null</code>.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @param encoding    The character encoding in which the output will be written.
     * @see   getEncoding
     * @since DOM Level 3
     */
    virtual void           setEncoding(const XMLCh* const encoding) = 0;

    /**
     * The end-of-line sequence of characters to be used in the XML being
     * written out. The only permitted values are these:
     * <dl>
     * <dt><code>null</code></dt>
     * <dd>
     * Use a default end-of-line sequence. DOM implementations should choose
     * the default to match the usual convention for text files in the
     * environment being used. Implementations must choose a default
     * sequence that matches one of those allowed by  2.11 "End-of-Line
     * Handling". </dd>
     * <dt>CR</dt>
     * <dd>The carriage-return character (#xD).</dd>
     * <dt>CR-LF</dt>
     * <dd> The
     * carriage-return and line-feed characters (#xD #xA). </dd>
     * <dt>LF</dt>
     * <dd> The line-feed
     * character (#xA). </dd>
     * </dl>
     * <br>The default value for this attribute is <code>null</code>.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @param newLine      The end-of-line sequence of characters to be used.
     * @see   getNewLine
     * @since DOM Level 3
     */
    virtual void          setNewLine(const XMLCh* const newLine) = 0;

    /**
     * The error handler that will receive error notifications during
     * serialization. The node where the error occured is passed to this
     * error handler, any modification to nodes from within an error
     * callback should be avoided since this will result in undefined,
     * implementation dependent behavior.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @param errorHandler The error handler to be used.
     * @see   getErrorHandler
     * @since DOM Level 3
     */
    virtual void         setErrorHandler(DOMErrorHandler *errorHandler) = 0;

    /**
     * When the application provides a filter, the serializer will call out
     * to the filter before serializing each Node. Attribute nodes are never
     * passed to the filter. The filter implementation can choose to remove
     * the node from the stream or to terminate the serialization early.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @param filter       The writer filter to be used.
     * @see   getFilter
     * @since DOM Level 3
     */
    virtual void         setFilter(DOMWriterFilter *filter) = 0;

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /**
     * Return the character encoding in which the output will be written.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @return The character encoding used.
     * @see   setEncoding
     * @since DOM Level 3
     */
     virtual const XMLCh*       getEncoding() const = 0;

    /**
     * Return the end-of-line sequence of characters to be used in the XML being
     * written out.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @return             The end-of-line sequence of characters to be used.
     * @see   setNewLine
     * @since DOM Level 3
     */
     virtual const XMLCh*       getNewLine() const = 0;

    /**
     * Return the error handler that will receive error notifications during
     * serialization.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @return             The error handler to be used.
     * @see   setErrorHandler
     * @since DOM Level 3
     */
     virtual DOMErrorHandler*   getErrorHandler() const = 0;

    /**
     * Return the WriterFilter used.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @return             The writer filter used.
     * @see   setFilter
     * @since DOM Level 3
     */
     virtual DOMWriterFilter*   getFilter() const = 0;

    // -----------------------------------------------------------------------
    //  Write methods
    // -----------------------------------------------------------------------
    /**
     * Write out the specified node as described above in the description of
     * <code>DOMWriter</code>. Writing a Document or Entity node produces a
     * serialized form that is well formed XML. Writing other node types
     * produces a fragment of text in a form that is not fully defined by
     * this document, but that should be useful to a human for debugging or
     * diagnostic purposes.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @param destination The destination for the data to be written.
     * @param nodeToWrite The <code>Document</code> or <code>Entity</code> node to
     *   be written. For other node types, something sensible should be
     *   written, but the exact serialized form is not specified.
     * @return  Returns <code>true</code> if <code>node</code> was
     *   successfully serialized and <code>false</code> in case a failure
     *   occured and the failure wasn't canceled by the error handler.
     * @since DOM Level 3
     */
    virtual bool       writeNode(XMLFormatTarget* const destination
                               , const DOMNode         &nodeToWrite) = 0;

    /**
     * Serialize the specified node as described above in the description of
     * <code>DOMWriter</code>. The result of serializing the node is
     * returned as a string. Writing a Document or Entity node produces a
     * serialized form that is well formed XML. Writing other node types
     * produces a fragment of text in a form that is not fully defined by
     * this document, but that should be useful to a human for debugging or
     * diagnostic purposes.
     *
     *  <p><b>"Experimental - subject to change"</b></p>
     *
     * @param nodeToWrite  The node to be written.
     * @return  Returns the serialized data, or <code>null</code> in case a
     *   failure occured and the failure wasn't canceled by the error
     *   handler.   The returned string is always in UTF-16.
     *   The encoding information available in DOMWriter is ignored in writeToString().
     * @since DOM Level 3
     */
    virtual XMLCh*     writeToString(const DOMNode &nodeToWrite) = 0;

    //@}

    // -----------------------------------------------------------------------
    //  Non-standard Extension
    // -----------------------------------------------------------------------
    /** @name Non-standard Extension */
    //@{
    /**
     * Called to indicate that this Writer is no longer in use
     * and that the implementation may relinquish any resources associated with it.
     *
     * Access to a released object will lead to unexpected result.
     */
    virtual void              release() = 0;
    //@}


};

XERCES_CPP_NAMESPACE_END

#endif
