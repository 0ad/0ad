/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2001 The Apache Software Foundation.  All rights
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
 * $Log: Attributes.hpp,v $
 * Revision 1.4  2003/03/07 18:10:30  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.3  2002/11/04 14:55:45  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/02/20 18:17:02  tng
 * [Bug 5977] Warnings on generating apiDocs.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:08  peiyongz
 * sane_include
 *
 * Revision 1.5  2001/05/11 13:26:25  tng
 * Copyright update.
 *
 * Revision 1.4  2001/02/26 19:44:19  tng
 * Schema: add utility class QName, by Pei Yong Zhang.
 *
 * Revision 1.3  2000/08/09 22:19:28  jpolast
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
 * Revision 1.2  2000/08/07 18:21:26  jpolast
 * change SAX_EXPORT module to SAX2_EXPORT
 *
 * Revision 1.1  2000/08/02 18:02:34  jpolast
 * initial checkin of sax2 implementation
 * submitted by Simon Fell (simon@fell.com)
 * and Joe Polastre (jpolast@apache.org)
 *
 *
 */

#ifndef ATTRIBUTES_HPP
#define ATTRIBUTES_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
  * Interface for an element's attribute specifications.
  *
  * The SAX2 parser implements this interface and passes an instance
  * to the SAX2 application as the last argument of each startElement
  * event.
  *
  * The instance provided will return valid results only during the
  * scope of the startElement invocation (to save it for future
  * use, the application must make a copy: the AttributesImpl
  * helper class provides a convenient constructor for doing so).
  *
  * An Attributes includes only attributes that have been
  * specified or defaulted: #IMPLIED attributes will not be included.
  *
  * There are two ways for the SAX application to obtain information
  * from the Attributes.  First, it can iterate through the entire
  * list:
  *
  * <pre>
  * public void startElement (String uri, String localpart, String qName, Attributes atts) {
  *   for (int i = 0; i < atts.getLength(); i++) {
  *     String Qname = atts.getQName(i);
  *		String URI   = atts.getURI(i)
  *		String local = atts.GetLocalName(i)
  *     String type  = atts.getType(i);
  *     String value = atts.getValue(i);
  *     [...]
  *   }
  * }
  * </pre>
  *
  * (Note that the result of getLength() will be zero if there
  * are no attributes.)
  *
  * As an alternative, the application can request the value or
  * type of specific attributes:
  *
  * <pre>
  * public void startElement (String uri, String localpart, String qName, Attributes atts) {
  *   String identifier = atts.getValue("id");
  *   String label = atts.getValue("label");
  *   [...]
  * }
  * </pre>
  *
  * The AttributesImpl helper class provides a convenience
  * implementation for use by parser or application writers.
  *
  * @see Sax2DocumentHandler#startElement
  * @see AttributesImpl#AttributesImpl
  */

class SAX2_EXPORT Attributes
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructors and Destructor */
    //@{
    /** Default constructor */
    Attributes()
    {
    }

    /** Destructor */
    virtual ~Attributes()
    {
    }
    //@}

    /** @name The virtual attribute list interface */
    //@{
  /**
    * Return the number of attributes in this list.
    *
    * The SAX parser may provide attributes in any
    * arbitrary order, regardless of the order in which they were
    * declared or specified.  The number of attributes may be
    * zero.
    *
    * @return The number of attributes in the list.
    */
    virtual unsigned int getLength() const = 0;

  /**
    * Return the namespace URI of an attribute in this list (by position).
    *
    * The QNames must be unique: the SAX parser shall not include the
    * same attribute twice.  Attributes without values (those declared
    * #IMPLIED without a value specified in the start tag) will be
    * omitted from the list.
    *
    * @param index The index of the attribute in the list (starting at 0).
    * @return The URI of the indexed attribute, or null
    *         if the index is out of range.
    * @see #getLength
    */
	virtual const XMLCh* getURI(const unsigned int index) const = 0;

  /**
    * Return the local name of an attribute in this list (by position).
    *
    * The QNames must be unique: the SAX parser shall not include the
    * same attribute twice.  Attributes without values (those declared
    * #IMPLIED without a value specified in the start tag) will be
    * omitted from the list.
    *
    * @param index The index of the attribute in the list (starting at 0).
    * @return The local name of the indexed attribute, or null
    *         if the index is out of range.
    * @see #getLength
    */
    virtual const XMLCh* getLocalName(const unsigned int index) const = 0;

  /**
    * Return the qName of an attribute in this list (by position).
    *
    * The QNames must be unique: the SAX parser shall not include the
    * same attribute twice.  Attributes without values (those declared
    * #IMPLIED without a value specified in the start tag) will be
    * omitted from the list.
    *
    * @param index The index of the attribute in the list (starting at 0).
    * @return The qName of the indexed attribute, or null
    *         if the index is out of range.
    * @see #getLength
    */
    virtual const XMLCh* getQName(const unsigned int index) const = 0;

  /**
    * Return the type of an attribute in the list (by position).
    *
    * The attribute type is one of the strings "CDATA", "ID",
    * "IDREF", "IDREFS", "NMTOKEN", "NMTOKENS", "ENTITY", "ENTITIES",
    * or "NOTATION" (always in upper case).
    *
    * If the parser has not read a declaration for the attribute,
    * or if the parser does not report attribute types, then it must
    * return the value "CDATA" as stated in the XML 1.0 Recommentation
    * (clause 3.3.3, "Attribute-Value Normalization").
    *
    * For an enumerated attribute that is not a notation, the
    * parser will report the type as "NMTOKEN".
    *
    * @param index The index of the attribute in the list (starting at 0).
    * @return The attribute type as a string, or
    *         null if the index is out of range.
    * @see #getLength
    * @see #getType(String)
    */
    virtual const XMLCh* getType(const unsigned int index) const = 0;

  /**
    * Return the value of an attribute in the list (by position).
    *
    * If the attribute value is a list of tokens (IDREFS,
    * ENTITIES, or NMTOKENS), the tokens will be concatenated
    * into a single string separated by whitespace.
    *
    * @param index The index of the attribute in the list (starting at 0).
    * @return The attribute value as a string, or
    *         null if the index is out of range.
    * @see #getLength
    * @see #getValue(XMLCh*)
    */
    virtual const XMLCh* getValue(const unsigned int index) const = 0;

    ////////////////////////////////////////////////////////////////////
    // Name-based query.
    ////////////////////////////////////////////////////////////////////

   /**
     * Look up the index of an attribute by Namespace name.
     *
     * @param uri The Namespace URI, or the empty string if
     *        the name has no Namespace URI.
     * @param localPart The attribute's local name.
     * @return The index of the attribute, or -1 if it does not
     *         appear in the list.
     */
	virtual int getIndex(const XMLCh* const uri, const XMLCh* const localPart ) const = 0 ;

   /**
     * Look up the index of an attribute by XML 1.0 qualified name.
     *
     * @param qName The qualified (prefixed) name.
     * @return The index of the attribute, or -1 if it does not
     *         appear in the list.
     */
	virtual int getIndex(const XMLCh* const qName ) const = 0 ;

   /**
     * Look up an attribute's type by Namespace name.
     *
     * <p>See #getType for a description of the possible types.</p>
     *
     * @param uri The Namespace URI, or the empty String if the
     *        name has no Namespace URI.
     * @param localPart The local name of the attribute.
     * @return The attribute type as a string, or null if the
     *         attribute is not in the list or if Namespace
     *         processing is not being performed.
     */
	virtual const XMLCh* getType(const XMLCh* const uri, const XMLCh* const localPart ) const = 0 ;

   /**
     * Look up an attribute's type by XML 1.0 qualified name.
     *
     * <p>See #getType for a description of the possible types.</p>
     *
     * @param qName The XML 1.0 qualified name.
     * @return The attribute type as a string, or null if the
     *         attribute is not in the list or if qualified names
     *         are not available.
     */
    virtual const XMLCh* getType(const XMLCh* const qName) const = 0;

   /**
     * Look up an attribute's value by Namespace name.
     *
     * <p>See #getValue for a description of the possible values.</p>
     *
     * @param uri The Namespace URI, or the empty String if the
     *        name has no Namespace URI.
     * @param localPart The local name of the attribute.
     * @return The attribute value as a string, or null if the
     *         attribute is not in the list.
     */
	virtual const XMLCh* getValue(const XMLCh* const uri, const XMLCh* const localPart ) const = 0 ;

   /**
     * Look up an attribute's value by XML 1.0 qualified name.
     *
     * <p>See #getValue for a description of the possible values.</p>
     *
     * @param qName The XML 1.0 qualified name.
     * @return The attribute value as a string, or null if the
     *         attribute is not in the list or if qualified names
     *         are not available.
     */
    virtual const XMLCh* getValue(const XMLCh* const qName) const = 0;

    //@}

private :
    /* Constructors and operators */
    /* Copy constructor */
    Attributes(const Attributes&);
    /* Assignment operator */
    Attributes& operator=(const Attributes&);

};

XERCES_CPP_NAMESPACE_END

#endif
