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
 * $Log: HandlerBase.hpp,v $
 * Revision 1.8  2004/01/29 11:46:32  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.7  2003/12/01 23:23:26  neilg
 * fix for bug 25118; thanks to Jeroen Witmond
 *
 * Revision 1.6  2003/05/30 16:11:44  gareth
 * Fixes so we compile under VC7.1. Patch by Alberto Massari.
 *
 * Revision 1.5  2002/11/04 14:56:25  tng
 * C++ Namespace Support.
 *
 * Revision 1.4  2002/07/16 18:15:00  tng
 * [Bug 6070] warning unused variable in HandlerBase.hpp
 *
 * Revision 1.3  2002/06/06 20:39:16  tng
 * Document Fix: document that the returned object from resolveEntity is owned by the parser
 *
 * Revision 1.2  2002/02/20 18:17:01  tng
 * [Bug 5977] Warnings on generating apiDocs.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:08  peiyongz
 * sane_include
 *
 * Revision 1.6  2000/03/02 19:54:35  roddey
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
 * Revision 1.1.1.1  1999/11/09 01:07:45  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:00  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#ifndef HANDLERBASE_HPP
#define HANDLERBASE_HPP

#include <xercesc/sax/DocumentHandler.hpp>
#include <xercesc/sax/DTDHandler.hpp>
#include <xercesc/sax/EntityResolver.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class Locator;
class AttributeList;

/**
  * Default base class for handlers.
  *
  * <p>This class implements the default behaviour for four SAX
  * interfaces: EntityResolver, DTDHandler, DocumentHandler,
  * and ErrorHandler.</p>
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
  * @see DocumentHandler#DocumentHandler
  * @see ErrorHandler#ErrorHandler
  */

class SAX_EXPORT HandlerBase :

    public EntityResolver, public DTDHandler, public DocumentHandler
    , public ErrorHandler
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
    * @param name The element type name.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see DocumentHandler#endElement
    */
    virtual void endElement(const XMLCh* const name);

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
    * @param name The element type name.
    * @param attributes The specified or defaulted attributes.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @see DocumentHandler#startElement
    */
    virtual void startElement
    (
        const   XMLCh* const    name
        ,       AttributeList&  attributes
    );

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

    HandlerBase() {};
    virtual ~HandlerBase() {};

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    HandlerBase(const HandlerBase&);
    HandlerBase& operator=(const HandlerBase&);
};


// ---------------------------------------------------------------------------
//  HandlerBase: Inline default implementations
// ---------------------------------------------------------------------------
inline void HandlerBase::characters(const   XMLCh* const
                                    , const unsigned int)
{
}

inline void HandlerBase::endDocument()
{
}

inline void HandlerBase::endElement(const XMLCh* const)
{
}

inline void HandlerBase::error(const SAXParseException&)
{
}

inline void HandlerBase::fatalError(const SAXParseException& exc)
{
    throw exc;
}

inline void
HandlerBase::ignorableWhitespace(   const   XMLCh* const
                                    , const unsigned int)
{
}

inline void HandlerBase::notationDecl(  const   XMLCh* const
                                        , const XMLCh* const
                                        , const XMLCh* const)
{
}

inline void
HandlerBase::processingInstruction( const   XMLCh* const
                                    , const XMLCh* const)
{
}

inline void HandlerBase::resetErrors()
{
}

inline void HandlerBase::resetDocument()
{
}

inline void HandlerBase::resetDocType()
{
}

inline InputSource*
HandlerBase::resolveEntity( const   XMLCh* const
                            , const XMLCh* const)
{
    return 0;
}

inline void
HandlerBase::unparsedEntityDecl(const   XMLCh* const
                                , const XMLCh* const
                                , const XMLCh* const
                                , const XMLCh* const)
{
}

inline void HandlerBase::setDocumentLocator(const Locator* const)
{
}

inline void HandlerBase::startDocument()
{
}

inline void
HandlerBase::startElement(  const   XMLCh* const
                            ,       AttributeList&)
{
}

inline void HandlerBase::warning(const SAXParseException&)
{
}

XERCES_CPP_NAMESPACE_END

#endif
