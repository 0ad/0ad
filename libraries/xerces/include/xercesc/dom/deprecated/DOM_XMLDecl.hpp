/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2002 The Apache Software Foundation.  All rights
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
 * $Id: DOM_XMLDecl.hpp,v 1.3 2002/11/04 15:04:44 tng Exp $
 *
*/

#ifndef DOM_XMLDecl_HEADER_GUARD_
#define DOM_XMLDecl_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_Node.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class XMLDeclImpl;
/**
* Class to refer to XML Declaration nodes in the DOM.
*
*/
class CDOM_EXPORT DOM_XMLDecl: public DOM_Node {

public:
    /** @name Constructors and assignment operators */
    //@{
    /**
     * The default constructor for DOM_XMLDecl creates a null
     * DOM_XMLDecl object that refers to a declaration node with
     * version= 1.0, encoding=utf-8 and standalone=no
     *
     */
    DOM_XMLDecl();

    /**
      * Copy constructor.  Creates a new <code>DOM_XMLDecl</code> that refers to the
      * same underlying actual xmlDecl node as the original.
      *
      * @param other The object to be copied
      */
    DOM_XMLDecl(const DOM_XMLDecl &other);
    /**
      * Assignment operator
      *
      * @param other The object to be copied
      */
    DOM_XMLDecl & operator = (const DOM_XMLDecl &other);

    /**
      * Assignment operator.  This overloaded variant is provided for
      *   the sole purpose of setting a DOM_Node reference variable to
      *   zero.  Nulling out a reference variable in this way will decrement
      *   the reference count on the underlying Node object that the variable
      *   formerly referenced.  This effect is normally obtained when reference
      *   variable goes out of scope, but zeroing them can be useful for
      *   global instances, or for local instances that will remain in scope
      *   for an extended time,  when the storage belonging to the underlying
      *   node needs to be reclaimed.
      *
      * @param val  Only a value of 0, or null, is allowed.
      */
    DOM_XMLDecl & operator = (const DOM_NullPtr *val);



	//@}
  /** @name Destructor */
  //@{
	
  /**
    * Destructor.  The object being destroyed is the reference
    * object, not the underlying Document itself.
    *
    * <p>The reference counting memory management will
    *  delete the underlying document itself if this
    * DOM_XMLDecl is the last remaining to refer to the Document,
    * and if there are no remaining references to any of the nodes
    * within the document tree.  If other live references do remain,
    * the underlying document itself remains also.
    *
    */
    ~DOM_XMLDecl();

  //@}

    //@{

  /**
    * To get the version string of the xmlDeclaration statement
    */
    DOMString getVersion() const;

  /**
    * To get the encoding string of the xmlDeclaration statement
    */
    DOMString getEncoding() const;

  /**
    * To get the standalone string of the xmlDeclaration statement
    */
    DOMString getStandalone() const;

   //@}

protected:
    DOM_XMLDecl( XMLDeclImpl *impl);

    friend class DOM_Document;

};


XERCES_CPP_NAMESPACE_END

#endif
