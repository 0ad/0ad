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
 * $Log: LexicalHandler.hpp,v $
 * Revision 1.3  2003/03/07 18:10:30  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.2  2002/11/04 14:55:45  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:09  peiyongz
 * sane_include
 *
 * Revision 1.1  2000/12/22 15:17:04  tng
 * SAX2-ext's LexicalHandler support added by David Bertoni.
 *
 *
 */


#ifndef LEXICALHANDLER_HPP
#define LEXICALHANDLER_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
  * Receive notification of lexical events.
  *
  * <p>This is an extension handler for that provides lexical information
  * about an XML document.  It does not provide information about document
  * content.  For those events, an application must register an instance of
  * a ContentHandler.</p>
  *
  * <p>The order of events in this interface is very important, and
  * mirrors the order of information in the document itself.  For
  * example, startDTD() and endDTD() events will occur before the
  * first element in the document.</p>
  *
  * @see SAX2XMLReader#setLexicalHandler
  * @see SAX2XMLReader#setContentHandler
  */

class SAX2_EXPORT LexicalHandler
{
public:
    /** @name Constructors and Destructor */
    //@{
    /** Default constructor */
    LexicalHandler()
    {
    }

    /** Destructor */
    virtual ~LexicalHandler()
    {
    }
    //@}

    /** @name The virtual document handler interface */

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
    ) = 0;

  /**
    * Receive notification of the end of a CDATA section.
    *
    * <p>The SAX parser will invoke this method at the end of
    * each CDATA parsed.</p>
    *
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void endCDATA () = 0;

  /**
    * Receive notification of the end of the DTD declarations.
    *
    * <p>The SAX parser will invoke this method at the end of the
    * DTD</p>
    *
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void endDTD () = 0;

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
    virtual void endEntity (const XMLCh* const name) = 0;

  /**
    * Receive notification of the start of a CDATA section.
    *
    * <p>The SAX parser will invoke this method at the start of
    * each CDATA parsed.</p>
    *
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    */
    virtual void startCDATA () = 0;

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
    ) = 0;

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
    virtual void startEntity (const XMLCh* const name) = 0;

    //@}
private :
    /* Unimplemented Constructors and operators */
    /* Copy constructor */
    LexicalHandler(const LexicalHandler&);
    /** Assignment operator */
    LexicalHandler& operator=(const LexicalHandler&);
};

XERCES_CPP_NAMESPACE_END

#endif
