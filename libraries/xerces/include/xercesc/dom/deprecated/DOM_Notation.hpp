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
 * $Id: DOM_Notation.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_Notation_HEADER_GUARD_
#define DOM_Notation_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_Node.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class NotationImpl;

/**
 * This interface represents a notation declared in the DTD. A notation either
 * declares, by name, the format of an unparsed entity (see section 4.7 of
 * the XML 1.0 specification), or is used for formal declaration of
 * Processing Instruction targets (see section 2.6 of the XML 1.0
 * specification). The <code>nodeName</code> attribute inherited from
 * <code>Node</code> is set to the declared name of the notation.
 * <p>The DOM Level 1 does not support editing <code>Notation</code> nodes;
 * they are therefore readonly.
 * <p>A <code>Notation</code> node does not have any parent.
 */
class DEPRECATED_DOM_EXPORT DOM_Notation: public DOM_Node {
public:
    /** @name Constructors and assignment operator */
    //@{
    /**
      * Default constructor for DOM_Notation.  The resulting object does not
    * refer to an actual Notation node; it will compare == to 0, and is similar
    * to a null object reference variable in Java.  It may subsequently be
    * assigned to refer to an actual Notation node.
    * <p>
    * New notation nodes are created by DOM_Document::createNotation().
    *
      *
      */
    DOM_Notation();

    /**
      * Copy constructor.  Creates a new <code>DOM_Notation</code> that refers to the
      * same underlying node as the original.  See also DOM_Node::clone(),
      * which will copy the actual notation node, rather than just creating a new
      * reference to the original node.
      *
      * @param other The object to be copied.
      */
    DOM_Notation(const DOM_Notation &other);

    /**
      * Assignment operator.
      *
      * @param other The object to be copied.
      */
    DOM_Notation & operator = (const DOM_Notation &other);

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
    DOM_Notation & operator = (const DOM_NullPtr *val);


    //@}
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOM_Notation.  The object being destroyed is the reference
      * object, not the underlying Notation node itself.
      *
	  */
    ~DOM_Notation();

    //@}
    /** @name Get functions. */
    //@{

    /**
     * Get the public identifier of this notation.
     *
     * If the  public identifier was not
     * specified, this is <code>null</code>.
     * @return Returns the public identifier of the notation
     */
    DOMString        getPublicId() const;
    /**
     * Get the system identifier of this notation.
     *
     * If the  system identifier was not
     * specified, this is <code>null</code>.
     * @return Returns the system identifier of the notation
     */
    DOMString        getSystemId() const;


    //@}

protected:
    DOM_Notation(NotationImpl *impl);

    friend class DOM_Document;

};

XERCES_CPP_NAMESPACE_END

#endif


