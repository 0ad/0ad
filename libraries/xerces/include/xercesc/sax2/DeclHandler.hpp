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
 * $Log: DeclHandler.hpp,v $
 * Revision 1.3  2003/03/07 18:10:30  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.2  2002/11/04 14:55:45  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:09  peiyongz
 * sane_include
 *
 * Revision 1.1  2002/01/28 17:08:33  knoaman
 * SAX2-ext's DeclHandler support.
 *
 */


#ifndef DECLHANDLER_HPP
#define DECLHANDLER_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
  * Receive notification of DTD declaration events.
  *
  * <p>This is an optional extension handler for SAX2 to provide more
  * complete information about DTD declarations in an XML document.
  * XML readers are not required to recognize this handler, and it is not
  * part of core-only SAX2 distributions.</p>
  *
  * <p>Note that data-related DTD declarations (unparsed entities and
  * notations) are already reported through the DTDHandler interface.</p>
  *
  * <p>If you are using the declaration handler together with a lexical
  * handler, all of the events will occur between the startDTD and the endDTD
  * events.</p>
  *
  * @see SAX2XMLReader#setLexicalHandler
  * @see SAX2XMLReader#setDeclarationHandler
  */

class SAX2_EXPORT DeclHandler
{
public:
    /** @name Constructors and Destructor */
    //@{
    /** Default constructor */
    DeclHandler()
    {
    }

    /** Destructor */
    virtual ~DeclHandler()
    {
    }
    //@}

    /** @name The virtual declaration handler interface */

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
    ) = 0;

   /**
    * Report an attribute type declaration.
    *
    * <p>The Parser will call this method to report each occurence of
    * a comment in the XML document.</p>
    *
    * <p>The application must not attempt to read from the array
    * outside of the specified range.</p>
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
    ) = 0;

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
    ) = 0;

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
    ) = 0;

    //@}
private :
    /* Unimplemented Constructors and operators */
    /* Copy constructor */
    DeclHandler(const DeclHandler&);
    /** Assignment operator */
    DeclHandler& operator=(const DeclHandler&);
};

XERCES_CPP_NAMESPACE_END

#endif
