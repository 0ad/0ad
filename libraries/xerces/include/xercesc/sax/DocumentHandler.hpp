/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2000 The Apache Software Foundation.  All rights
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
 * $Log: DocumentHandler.hpp,v $
 * Revision 1.3  2003/03/07 18:10:06  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.2  2002/11/04 14:56:25  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:08  peiyongz
 * sane_include
 *
 * Revision 1.6  2000/03/02 19:54:34  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.5  2000/02/24 20:12:55  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.4  2000/02/12 03:31:55  rahulj
 * Removed duplicate CVS Log entries.
 *
 * Revision 1.3  2000/02/12 01:27:19  aruna1
 * Documentation updated
 *
 * Revision 1.2  2000/02/06 07:47:57  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:07:43  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:44:54  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#ifndef DOCUMENTHANDLER_HPP
#define DOCUMENTHANDLER_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class AttributeList;
class Locator;

/**
  * Receive notification of general document events.
  *
  * <p>This is the main interface that most SAX applications
  * implement: if the application needs to be informed of basic parsing
  * events, it implements this interface and registers an instance with
  * the SAX parser using the setDocumentHandler method.  The parser
  * uses the instance to report basic document-related events like
  * the start and end of elements and character data.</p>
  *
  * <p>The order of events in this interface is very important, and
  * mirrors the order of information in the document itself.  For
  * example, all of an element's content (character data, processing
  * instructions, and/or subelements) will appear, in order, between
  * the startElement event and the corresponding endElement event.</p>
  *
  * <p>Application writers who do not want to implement the entire
  * interface while can derive a class from HandlerBase, which implements
  * the default functionality; parser writers can instantiate
  * HandlerBase to obtain a default handler.  The application can find
  * the location of any document event using the Locator interface
  * supplied by the Parser through the setDocumentLocator method.</p>
  *
  * @see Parser#setDocumentHandler
  * @see Locator#Locator
  * @see HandlerBase#HandlerBase
  */

class SAX_EXPORT DocumentHandler
{
public:
    /** @name Constructors and Destructor */
    //@{
    /** Default constructor */
    DocumentHandler()
    {
    }

    /** Destructor */
    virtual ~DocumentHandler()
    {
    }
    //@}

    /** @name The virtual document handler interface */

    //@{
   /**
    * Receive notification of character data.
    *
    * <p>The Parser will call this method to report each chunk of
    * character data.  SAX parsers may return all contiguous character
    * data in a single chunk, or they may split it into several
    * chunks; however, all of the characters in any single event
    * must come from the same external entity, so that the Locator
    * provides useful information.</p>
    *
    * <p>The application must not attempt to read from the array
    * outside of the specified range.</p>
    *
    * <p>Note that some parsers will report whitespace using the
    * ignorableWhitespace() method rather than this one (validating
    * parsers must do so).</p>
    *
    * @param chars The characters from the XML document.
    * @param length The number of characters to read from the array.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see #ignorableWhitespace
    * @see Locator#Locator
    */
    virtual void characters
    (
        const   XMLCh* const    chars
        , const unsigned int    length
    ) = 0;

  /**
    * Receive notification of the end of a document.
    *
    * <p>The SAX parser will invoke this method only once, and it will
    * be the last method invoked during the parse.  The parser shall
    * not invoke this method until it has either abandoned parsing
    * (because of an unrecoverable error) or reached the end of
    * input.</p>
    *
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void endDocument () = 0;

  /**
    * Receive notification of the end of an element.
    *
    * <p>The SAX parser will invoke this method at the end of every
    * element in the XML document; there will be a corresponding
    * startElement() event for every endElement() event (even when the
    * element is empty).</p>
    *
    * <p>If the element name has a namespace prefix, the prefix will
    * still be attached to the name.</p>
    *
    * @param name The element type name
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void endElement(const XMLCh* const name) = 0;

  /**
    * Receive notification of ignorable whitespace in element content.
    *
    * <p>Validating Parsers must use this method to report each chunk
    * of ignorable whitespace (see the W3C XML 1.0 recommendation,
    * section 2.10): non-validating parsers may also use this method
    * if they are capable of parsing and using content models.</p>
    *
    * <p>SAX parsers may return all contiguous whitespace in a single
    * chunk, or they may split it into several chunks; however, all of
    * the characters in any single event must come from the same
    * external entity, so that the Locator provides useful
    * information.</p>
    *
    * <p>The application must not attempt to read from the array
    * outside of the specified range.</p>
    *
    * @param chars The characters from the XML document.
    * @param length The number of characters to read from the array.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see #characters
    */
    virtual void ignorableWhitespace
    (
        const   XMLCh* const    chars
        , const unsigned int    length
    ) = 0;

  /**
    * Receive notification of a processing instruction.
    *
    * <p>The Parser will invoke this method once for each processing
    * instruction found: note that processing instructions may occur
    * before or after the main document element.</p>
    *
    * <p>A SAX parser should never report an XML declaration (XML 1.0,
    * section 2.8) or a text declaration (XML 1.0, section 4.3.1)
    * using this method.</p>
    *
    * @param target The processing instruction target.
    * @param data The processing instruction data, or null if
    *        none was supplied.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void processingInstruction
    (
        const   XMLCh* const    target
        , const XMLCh* const    data
    ) = 0;

    /**
    * Reset the Docuemnt object on its reuse
    *
    * <p>This method helps in reseting the document implementational
    * defaults each time the document is begun.</p>
    *
    */
    virtual void resetDocument() = 0;

  /**
    * Receive an object for locating the origin of SAX document events.
    *
    * SAX parsers are strongly encouraged (though not absolutely
    * required) to supply a locator: if it does so, it must supply
    * the locator to the application by invoking this method before
    * invoking any of the other methods in the DocumentHandler
    * interface.
    *
    * The locator allows the application to determine the end
    * position of any document-related event, even if the parser is
    * not reporting an error.  Typically, the application will
    * use this information for reporting its own errors (such as
    * character content that does not match an application's
    * business rules). The information returned by the locator
    * is probably not sufficient for use with a search engine.
    *
    * Note that the locator will return correct information only
    * during the invocation of the events in this interface. The
    * application should not attempt to use it at any other time.
    *
    * @param locator An object that can return the location of
    *                any SAX document event. The object is only
    *                'on loan' to the client code and they are not
    *                to attempt to delete or modify it in any way!
    *
    * @see Locator#Locator
    */
    virtual void setDocumentLocator(const Locator* const locator) = 0;

  /**
    * Receive notification of the beginning of a document.
    *
    * <p>The SAX parser will invoke this method only once, before any
    * other methods in this interface or in DTDHandler (except for
    * setDocumentLocator).</p>
    *
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void startDocument() = 0;

  /**
    * Receive notification of the beginning of an element.
    *
    * <p>The Parser will invoke this method at the beginning of every
    * element in the XML document; there will be a corresponding
    * endElement() event for every startElement() event (even when the
    * element is empty). All of the element's content will be
    * reported, in order, before the corresponding endElement()
    * event.</p>
    *
    * <p>If the element name has a namespace prefix, the prefix will
    * still be attached.  Note that the attribute list provided will
    * contain only attributes with explicit values (specified or
    * defaulted): #IMPLIED attributes will be omitted.</p>
    *
    * @param name The element type name.
    * @param attrs The attributes attached to the element, if any.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see #endElement
    * @see AttributeList#AttributeList
    */
    virtual void startElement
    (
        const   XMLCh* const    name
        ,       AttributeList&  attrs
    ) = 0;

    //@}

private :
    /* Unimplemented Constructors and operators */
    /* Copy constructor */
    DocumentHandler(const DocumentHandler&);
    /** Assignment operator */
    DocumentHandler& operator=(const DocumentHandler&);
};

XERCES_CPP_NAMESPACE_END

#endif
