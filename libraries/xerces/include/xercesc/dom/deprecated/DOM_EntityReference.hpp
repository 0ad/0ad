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
 * $Id: DOM_EntityReference.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_EntityReference_HEADER_GUARD_
#define DOM_EntityReference_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_Node.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class EntityReferenceImpl;

/**
 * <code>EntityReference</code> nodes will appear in the structure
 * model when an entity reference is in the source document, or when the user
 * wishes to insert an entity reference.
 *
 * The expansion of the entity will appear as child nodes of the entity
 * reference node.  The expansion may be just simple text, or it may
 * be more complex, containing additional entity refs.
 *
*/

class DEPRECATED_DOM_EXPORT DOM_EntityReference: public DOM_Node {
public:
    /** @name Constructors and assignment operator */
    //@{
    /**
      * Default constructor for DOM_EntityReference.  The resulting object does not
    * refer to an actual Entity Reference node; it will compare == to 0, and is similar
    * to a null object reference variable in Java.  It may subsequently be
    * assigned to refer to an actual entity reference node.
    * <p>
    * New entity reference nodes are created by DOM_Document::createEntityReference().
      *
      */
    DOM_EntityReference();

    /**
      * Copy constructor.  Creates a new <code>DOM_EntityReference</code> that refers to the
    * same underlying node as the original.
      *
      * @param other The object to be copied.
      */
    DOM_EntityReference(const DOM_EntityReference &other);

    /**
      * Assignment operator.
      *
      * @param other The object to be copied.
      */
    DOM_EntityReference & operator = (const DOM_EntityReference &other);

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
    DOM_EntityReference & operator = (const DOM_NullPtr *val);

    //@}
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOM_EntityReference.  The object being destroyed is the reference
      * object, not the underlying entity reference node itself.
	  *
	  */
    ~DOM_EntityReference();
    //@}

protected:
    DOM_EntityReference(EntityReferenceImpl *impl);

    friend class DOM_Document;
};

XERCES_CPP_NAMESPACE_END

#endif


