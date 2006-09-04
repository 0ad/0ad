/*
 * Copyright 1999-2002,2004 The Apache Software Foundation.
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
 * $Id: DOM_XMLDecl.hpp 176026 2004-09-08 13:57:07Z peiyongz $
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
class DEPRECATED_DOM_EXPORT DOM_XMLDecl: public DOM_Node {

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
