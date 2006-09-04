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
 * $Id: DOM_Comment.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_Comment_HEADER_GUARD_
#define DOM_Comment_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_CharacterData.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class CommentImpl;

/**
 * Class to refer to XML comment nodes in the DOM.
 *
 * <P>The string value contains all of the characters between
 * the starting '<code>&lt;!--</code>' and ending '<code>--&gt;</code>'.
 */
class DEPRECATED_DOM_EXPORT DOM_Comment: public DOM_CharacterData {

public:
  /** @name Constructors and assignment operators */
  //@{
  /**
    * Default constructor for DOM_Comment.  The resulting object does not
    * refer to an actual Comment node; it will compare == to 0, and is similar
    * to a null object reference variable in Java.  It may subsequently be
    * assigned to refer to an actual comment node.
    * <p>
    * New comment nodes are created by DOM_Document::createComment().
    *
    */
    DOM_Comment();

  /**
    * Copy constructor.   Creates a new <code>DOM_Comment</code> that refers to the
    * same underlying node as the original.  See also DOM_Node::clone(),
    * which will copy the actual Comment node, rather than just creating a new
    * reference to the original node.
    *
    * @param other The object to be copied.
    */
    DOM_Comment(const DOM_Comment &other);
  /**
    * Assignment operator.
    *
    * @param other The object to be copied.
    */
    DOM_Comment & operator = (const DOM_Comment &other);

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
      * @param val   Only a value of 0, or null, is allowed.
      */
    DOM_Comment & operator = (const DOM_NullPtr *val);



    //@}
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOM_Comment.  The object being destroyed is the reference
      * object, not the underlying Comment node itself.
      *
	  */
    ~DOM_Comment();
    //@}

protected:
    DOM_Comment(CommentImpl *comment);

    friend class DOM_Document;



};

XERCES_CPP_NAMESPACE_END

#endif

