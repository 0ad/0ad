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
 * $Log: DefaultHandler.hpp,v $
 * Revision 1.7  2004/01/29 11:46:32  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.6  2003/12/02 14:33:13  amassari
 * Don't use the word "exception" as a variable name, as VC 7.1 complains about it
 *
 * Revision 1.5  2002/11/04 14:55:45  tng
 * C++ Namespace Support.
 *
 * Revision 1.4  2002/08/20 16:53:56  tng
 * [Bug 6251] Info during compilation.
 *
 * Revision 1.3  2002/06/06 20:38:45  tng
 * Document Fix: document that the returned object from resolveEntity is owned by the parser
 *
 * Revision 1.2  2002/02/20 18:17:02  tng
 * [Bug 5977] Warnings on generating apiDocs.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:09  peiyongz
 * sane_include
 *
 * Revision 1.6  2002/01/28 17:08:33  knoaman
 * SAX2-ext's DeclHandler support.
 *
 * Revision 1.5  2001/11/13 13:24:05  tng
 * Fix documentation for DefaultHandler.
 *
 * Revision 1.4  2000/12/22 15:17:01  tng
 * SAX2-ext's LexicalHandler support added by David Bertoni.
 *
 * Revision 1.3  2000/08/14 18:56:14  aruna1
 * Virtual parameter inconsistency fixed
 *
 * Revision 1.2  2000/08/07 18:21:27  jpolast
 * change SAX_EXPORT module to SAX2_EXPORT
 *
 * Revision 1.1  2000/08/02 18:02:34  jpolast
 * initial checkin of sax2 implementation
 * submitted by Simon Fell (simon@fell.com)
 * and Joe Polastre (jpolast@apache.org)
 *
 *
 */


#ifndef DEFAULTHANDLER_HPP
#define DEFAULTHANDLER_HPP

#include <xercesc/sax2/ContentHandler.hpp>
#include <xercesc/sax2/LexicalHandler.hpp>
#include <xercesc/sax2/DeclHandler.hpp>
#include <xercesc/sax/DTDHandler.hpp>
#include <xercesc/sax/EntityResolver.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class Locator;
class Attributes;

/**
  * Default base class for SAX2 handlers.
  *
  * <p>This class implements the default behaviour for SAX2
  * interfaces: EntityResolver, DTDHandler, ContentHandler,
  * ErrorHandler, LexicalHandler, and DeclHandler.</p>
  *
  * <p>Application writers can extend this class when they need to
  * implement only part of an interface; parser writers can
  * instantiate this class to provide default handlers when the
  * application has not supplied its own.</p>
  *
  * <p>Note that the use of this class is optional.</p>
  *
  * @see EntityResolver#EntityResolver
  * @see DTDHandler#DTDHandler
  * @see ContentHandler#ContentHandler
  * @see ErrorHandler#ErrorHandler
  * @see LexicalHandler#LexicalHandler
  * @see DeclHandler#DeclHandler
  */

class SAX2_EXPORT DefaultHandler :

    public EntityResolver,
	public DTDHandler,
	public ContentHandler,
    public ErrorHandler,
    public LexicalHandler,
    public DeclHandler
{
public:
  /** @name Default handlers for the DocumentHandler interface */
    //@{
  /**
    * Receive notification of character data inside an element.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method to take specific actions for each chunk of character data
    * (such as adding the data to a node or buffer, or printing it to
    * a file).</p>
    *
    * @param chars The characters.
    * @param length The number of characters to use from the
    *               character array.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see DocumentHandler#characters
    */
    virtual void characters
    (
        const   XMLCh* const    chars
        , const unsigned int    length
    );

  /**
    * Receive notification of the end of the document.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method in a subclass to take specific actions at the beginning
    * of a document (such as finalising a tree or closing an output
    * file).</p>
    *
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see DocumentHandler#endDocument
    */
    virtual void endDocument();

  /**
    * Receive notification of the end of an element.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method in a subclass to take specific actions at the end of
    * each element (such as finalising a tree node or writing
    * output to a file).</p>
    *
    * @param uri The URI of the asscioated namespace for this element
	* @param localname The local part of the element name
	* @param qname The QName of this element
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see DocumentHandler#endElement
    */
    virtual void endElement
	(
		const XMLCh* const uri,
		const XMLCh* const localname,
		const XMLCh* const qname
	);

  /**
    * Receive notification of ignorable whitespace in element content.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method to take specific actions for each chunk of ignorable
    * whitespace (such as adding data to a node or buffer, or printing
    * it to a file).</p>
    *
    * @param chars The whitespace characters.
    * @param length The number of characters to use from the
    *               character array.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see DocumentHandler#ignorableWhitespace
    */
    virtual void ignorableWhitespace
    (
        const   XMLCh* const    chars
        , const unsigned int    length
    );

  /**
    * Receive notification of a processing instruction.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method in a subclass to take specific actions for each
    * processing instruction, such as setting status variables or
    * invoking other methods.</p>
    *
    * @param target The processing instruction target.
    * @param data The processing instruction data, or null if
    *             none is supplied.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see DocumentHandler#processingInstruction
    */
    virtual void processingInstruction
    (
        const   XMLCh* const    target
        , const XMLCh* const    data
    );

    /**
    * Reset the Docuemnt object on its reuse
    *
    * @see DocumentHandler#resetDocument
    */
    virtual void resetDocument();
    //@}

    /** @name Default implementation of DocumentHandler interface */

    //@{
  /**
    * Receive a Locator object for document events.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method in a subclass if they wish to store the locator for use
    * with other document events.</p>
    *
    * @param locator A locator for all SAX document events.
    * @see DocumentHandler#setDocumentLocator
    * @see Locator
    */
    virtual void setDocumentLocator(const Locator* const locator);

  /**
    * Receive notification of the beginning of the document.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method in a subclass to take specific actions at the beginning
    * of a document (such as allocating the root node of a tree or
    * creating an output file).</p>
    *
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see DocumentHandler#startDocument
    */
    virtual void startDocument();

  /**
    * Receive notification of the start of an element.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method in a subclass to take specific actions at the start of
    * each element (such as allocating a new tree node or writing
    * output to a file).</p>
    *
    * @param uri The URI of the asscioated namespace for this element
	* @param localname the local part of the element name
	* @param qname the QName of this element
    * @param attrs The specified or defaulted attributes.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see DocumentHandler#startElement
    */
    virtual void startElement
    (
        const   XMLCh* const    uri,
        const   XMLCh* const    localname,
        const   XMLCh* const    qname
        , const Attributes&	attrs
    );

  /**
    * Receive notification of the start of an namespace prefix mapping.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method in a subclass to take specific actions at the start of
    * each namespace prefix mapping.</p>
    *
	* NOTE : The current Implementation of SAX2Parser will _NEVER_ call this
	*
    * @param prefix The namespace prefix used
    * @param uri The namespace URI used.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see DocumentHandler#startPrefixMapping
    */
	virtual void startPrefixMapping
	(
		const	XMLCh* const	prefix,
		const	XMLCh* const	uri
	) ;

  /**
    * Receive notification of the end of an namespace prefix mapping.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method in a subclass to take specific actions at the end of
    * each namespace prefix mapping.</p>
    *
	* NOTE : The current Implementation of SAX2Parser will _NEVER_ call this
	*
    * @param prefix The namespace prefix used
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see DocumentHandler#endPrefixMapping
    */
	virtual void endPrefixMapping
	(
		const	XMLCh* const	prefix
	) ;

  /**
    * Receive notification of a skipped entity
    *
    * <p>The parser will invoke this method once for each entity
	* skipped.  All processors may skip external entities,
	* depending on the values of the features:<br>
	* http://xml.org/sax/features/external-general-entities<br>
	* http://xml.org/sax/features/external-parameter-entities</p>
    *
	* <p>Introduced with SAX2</p>
	*
    * @param name The name of the skipped entity.  If it is a parameter entity,
	* the name will begin with %, and if it is the external DTD subset,
	* it will be the string [dtd].
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
	virtual void skippedEntity
	(
		const	XMLCh* const	name
	) ;

    //@}

    /** @name Default implementation of the EntityResolver interface. */

    //@{
  /**
    * Resolve an external entity.
    *
    * <p>Always return null, so that the parser will use the system
    * identifier provided in the XML document.  This method implements
    * the SAX default behaviour: application writers can override it
    * in a subclass to do special translations such as catalog lookups
    * or URI redirection.</p>
    *
    * @param publicId The public identifer, or null if none is
    *                 available.
    * @param systemId The system identifier provided in the XML
    *                 document.
    * @return The new input source, or null to require the
    *         default behaviour.
    *         The returned InputSource is owned by the parser which is
    *         responsible to clean up the memory.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see EntityResolver#resolveEntity
    */
    virtual InputSource* resolveEntity
    (
        const   XMLCh* const    publicId
        , const XMLCh* const    systemId
    );

    //@}

    /** @name Default implementation of the ErrorHandler interface */
    //@{
   /**
    * Receive notification of a recoverable parser error.
    *
    * <p>The default implementation does nothing.  Application writers
    * may override this method in a subclass to take specific actions
    * for each error, such as inserting the message in a log file or
    * printing it to the console.</p>
    *
    * @param exc The warning information encoded as an exception.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see ErrorHandler#warning
    * @see SAXParseException#SAXParseException
    */
    virtual void error(const SAXParseException& exc);

  /**
    * Report a fatal XML parsing error.
    *
    * <p>The default implementation throws a SAXParseException.
    * Application writers may override this method in a subclass if
    * they need to take specific actions for each fatal error (such as
    * collecting all of the errors into a single report): in any case,
    * the application must stop all regular processing when this
    * method is invoked, since the document is no longer reliable, and
    * the parser may no longer report parsing events.</p>
    *
    * @param exc The error information encoded as an exception.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see ErrorHandler#fatalError
    * @see SAXParseException#SAXParseException
    */
    virtual void fatalError(const SAXParseException& exc);

  /**
    * Receive notification of a parser warning.
    *
    * <p>The default implementation does nothing.  Application writers
    * may override this method in a subclass to take specific actions
    * for each warning, such as inserting the message in a log file or
    * printing it to the console.</p>
    *
    * @param exc The warning information encoded as an exception.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see ErrorHandler#warning
    * @see SAXParseException#SAXParseException
    */
    virtual void warning(const SAXParseException& exc);

    /**
    * Reset the Error handler object on its reuse
    *
    * @see ErrorHandler#resetErrors
    */
    virtual void resetErrors();

    //@}


    /** @name Default implementation of DTDHandler interface. */
    //@{

  /**
    * Receive notification of a notation declaration.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method in a subclass if they wish to keep track of the notations
    * declared in a document.</p>
    *
    * @param name The notation name.
    * @param publicId The notation public identifier, or null if not
    *                 available.
    * @param systemId The notation system identifier.
    * @see DTDHandler#notationDecl
    */
    virtual void notationDecl
    (
        const   XMLCh* const    name
        , const XMLCh* const    publicId
        , const XMLCh* const    systemId
    );

    /**
    * Reset the DTD object on its reuse
    *
    * @see DTDHandler#resetDocType
    */
    virtual void resetDocType();

  /**
    * Receive notification of an unparsed entity declaration.
    *
    * <p>By default, do nothing.  Application writers may override this
    * method in a subclass to keep track of the unparsed entities
    * declared in a document.</p>
    *
    * @param name The entity name.
    * @param publicId The entity public identifier, or null if not
    *                 available.
    * @param systemId The entity system identifier.
    * @param notationName The name of the associated notation.
    * @see DTDHandler#unparsedEntityDecl
    */
    virtual void unparsedEntityDecl
    (
        const   XMLCh* const    name
        , const XMLCh* const    publicId
        , const XMLCh* const    systemId
        , const XMLCh* const    notationName
    );
    //@}


    /** @name Default implementation of LexicalHandler interface. */

    //@{
   /**
    * Receive notification of comments.
    *
    * <p>The Parser will call this method to report each occurence of
    * a comment in the XML document.</p>
    *
    * <p>The application must not attempt to read from the array
    * outside of the specified range.</p>
    *
    * @param chars The characters from the XML document.
    * @param length The number of characters to read from the array.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void comment
    (
        const   XMLCh* const    chars
        , const unsigned int    length
    );

  /**
    * Receive notification of the end of a CDATA section.
    *
    * <p>The SAX parser will invoke this method at the end of
    * each CDATA parsed.</p>
    *
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void endCDATA ();

  /**
    * Receive notification of the end of the DTD declarations.
    *
    * <p>The SAX parser will invoke this method at the end of the
    * DTD</p>
    *
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void endDTD ();

  /**
    * Receive notification of the end of an entity.
    *
    * <p>The SAX parser will invoke this method at the end of an
    * entity</p>
    *
    * @param name The name of the entity that is ending.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void endEntity (const XMLCh* const name);

  /**
    * Receive notification of the start of a CDATA section.
    *
    * <p>The SAX parser will invoke this method at the start of
    * each CDATA parsed.</p>
    *
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void startCDATA ();

  /**
    * Receive notification of the start of the DTD declarations.
    *
    * <p>The SAX parser will invoke this method at the start of the
    * DTD</p>
    *
    * @param name The document type name.
    * @param publicId The declared public identifier for the external DTD subset, or null if none was declared.
    * @param systemId The declared system identifier for the external DTD subset, or null if none was declared.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void startDTD
    (
        const   XMLCh* const    name
        , const   XMLCh* const    publicId
        , const   XMLCh* const    systemId
    );

  /**
    * Receive notification of the start of an entity.
    *
    * <p>The SAX parser will invoke this method at the start of an
    * entity</p>
    *
    * @param name The name of the entity that is starting.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void startEntity (const XMLCh* const name);

    //@}

    /** @name Default implementation of DeclHandler interface. */

    //@{

   /**
    * Report an element type declaration.
    *
    * <p>The content model will consist of the string "EMPTY", the string
    * "ANY", or a parenthesised group, optionally followed by an occurrence
    * indicator. The model will be normalized so that all parameter entities
    * are fully resolved and all whitespace is removed,and will include the
    * enclosing parentheses. Other normalization (such as removing redundant
    * parentheses or simplifying occurrence indicators) is at the discretion
    * of the parser.</p>
    *
    * @param name The element type name.
    * @param model The content model as a normalized string.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void elementDecl
    (
        const   XMLCh* const    name
        , const XMLCh* const    model
    );

   /**
    * Report an attribute type declaration.
    *
    * <p>Only the effective (first) declaration for an attribute will
    * be reported.</p>
    *
    * @param eName The name of the associated element.
    * @param aName The name of the attribute.
    * @param type A string representing the attribute type.
    * @param mode A string representing the attribute defaulting mode ("#IMPLIED", "#REQUIRED", or "#FIXED") or null if none of these applies.
    * @param value A string representing the attribute's default value, or null if there is none.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void attributeDecl
    (
        const   XMLCh* const    eName
        , const XMLCh* const    aName
        , const XMLCh* const    type
        , const XMLCh* const    mode
        , const XMLCh* const    value
    );

   /**
    * Report an internal entity declaration.
    *
    * <p>Only the effective (first) declaration for each entity will be
    * reported. All parameter entities in the value will be expanded, but
    * general entities will not.</p>
    *
    * @param name The name of the entity. If it is a parameter entity, the name will begin with '%'.
    * @param value The replacement text of the entity.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void internalEntityDecl
    (
        const   XMLCh* const    name
        , const XMLCh* const    value
    );

   /**
    * Report a parsed external entity declaration.
    *
    * <p>Only the effective (first) declaration for each entity will
    * be reported.</p>
    *
    * @param name The name of the entity. If it is a parameter entity, the name will begin with '%'.
    * @param publicId The The declared public identifier of the entity, or null if none was declared.
    * @param systemId The declared system identifier of the entity.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void externalEntityDecl
    (
        const   XMLCh* const    name
        , const XMLCh* const    publicId
        , const XMLCh* const    systemId
    );

    //@}

    DefaultHandler() {};
    virtual ~DefaultHandler() {};

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    DefaultHandler(const DefaultHandler&);
    DefaultHandler& operator=(const DefaultHandler&);    
};


// ---------------------------------------------------------------------------
//  HandlerBase: Inline default implementations
// ---------------------------------------------------------------------------
inline void DefaultHandler::characters(const   XMLCh* const
                                       ,const   unsigned int)
{
}

inline void DefaultHandler::endDocument()
{
}

inline void DefaultHandler::endElement(const	XMLCh* const
										, const XMLCh* const
										, const XMLCh* const)
{
}

inline void DefaultHandler::error(const SAXParseException&)
{
}

inline void DefaultHandler::fatalError(const SAXParseException& exc)
{
    throw exc;
}

inline void
DefaultHandler::ignorableWhitespace( const   XMLCh* const
                                    , const unsigned int)
{
}

inline void DefaultHandler::notationDecl(  const   XMLCh* const
											, const XMLCh* const
											, const XMLCh* const)
{
}

inline void
DefaultHandler::processingInstruction( const   XMLCh* const
										, const XMLCh* const)
{
}

inline void DefaultHandler::resetErrors()
{
}

inline void DefaultHandler::resetDocument()
{
}

inline void DefaultHandler::resetDocType()
{
}

inline InputSource*
DefaultHandler::resolveEntity( const   XMLCh* const
								, const XMLCh* const)
{
    return 0;
}

inline void
DefaultHandler::unparsedEntityDecl(const   XMLCh* const
									, const XMLCh* const
									, const XMLCh* const
									, const XMLCh* const)
{
}

inline void DefaultHandler::setDocumentLocator(const Locator* const)
{
}

inline void DefaultHandler::startDocument()
{
}

inline void
DefaultHandler::startElement(  const     XMLCh* const
								, const   XMLCh* const
								, const   XMLCh* const
								, const   Attributes&
)
{
}

inline void DefaultHandler::warning(const SAXParseException&)
{
}

inline void DefaultHandler::startPrefixMapping ( const	XMLCh* const
												,const	XMLCh* const)
{
}

inline void DefaultHandler::endPrefixMapping ( const	XMLCh* const)
{
}

inline void DefaultHandler::skippedEntity ( const	XMLCh* const)
{
}

inline void DefaultHandler::comment(  const   XMLCh* const
                                       , const unsigned int)
{
}

inline void DefaultHandler::endCDATA ()
{
}

inline void DefaultHandler::endDTD ()
{
}

inline void DefaultHandler::endEntity (const XMLCh* const)
{
}

inline void DefaultHandler::startCDATA ()
{
}

inline void DefaultHandler::startDTD(  const   XMLCh* const
                                        , const   XMLCh* const
                                        , const   XMLCh* const)
{
}

inline void DefaultHandler::startEntity (const XMLCh* const)
{
}

inline void DefaultHandler::attributeDecl(const XMLCh* const,
                                          const XMLCh* const,
                                          const XMLCh* const,
                                          const XMLCh* const,
                                          const XMLCh* const)
{
}

inline void DefaultHandler::elementDecl(const XMLCh* const,
                                        const XMLCh* const)
{
}

inline void DefaultHandler::externalEntityDecl(const XMLCh* const,
                                               const XMLCh* const,
                                               const XMLCh* const)
{
}

inline void DefaultHandler::internalEntityDecl(const XMLCh* const,
                                               const XMLCh* const)
{
}

XERCES_CPP_NAMESPACE_END

#endif // ! DEFAULTHANDLER_HPP
