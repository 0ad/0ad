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
 * $Id: DOM_Entity.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_Entity_HEADER_GUARD_
#define DOM_Entity_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_Node.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class EntityImpl;

/**
 * This interface represents an entity, either parsed or unparsed, in an XML
 * document.
 *
 * Note that this models the entity itself not the entity
 * declaration. <code>Entity</code> declaration modeling has been left for a
 * later Level of the DOM specification.
 * <p>The <code>nodeName</code> attribute that is inherited from
 * <code>Node</code> contains the name of the entity.
 * <p>An XML processor may choose to completely expand entities before  the
 * structure model is passed to the DOM; in this case there will be no
 * <code>EntityReference</code> nodes in the document tree.
 *
 * <p>Note: the first release of this parser does not create entity
 *    nodes when reading an XML document.  Entities may be
 *    programatically created using DOM_Document::createEntity().
 */
class DEPRECATED_DOM_EXPORT DOM_Entity: public DOM_Node {
public:
    /** @name Constructors and assignment operator */
    //@{
    /**
      * Default constructor for DOM_Entity.
      *
      */
    DOM_Entity();

    /**
      * Copy constructor.
      *
      * @param other The object to be copied.
      */
    DOM_Entity(const DOM_Entity &other);

    /**
      * Assignment operator.
      *
      * @param other The object to be copied.
      */
    DOM_Entity & operator = (const DOM_Entity &other);

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
    DOM_Entity & operator = (const DOM_NullPtr *val);

    //@}
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOM_Entity.
	  *
	  */
    ~DOM_Entity();

    //@}
    /** @name Get functions. */
    //@{
  /**
   * The public identifier associated with the entity, if specified.
   *
   * If the public identifier was not specified, this is <code>null</code>.
   */
  DOMString        getPublicId() const;

  /**
   * The system identifier associated with the entity, if specified.
   *
   * If the system identifier was not specified, this is <code>null</code>.
   */
  DOMString        getSystemId() const;

  /**
   * For unparsed entities, the name of the notation for the entity.
   *
   * For parsed entities, this is <code>null</code>.
   */
  DOMString        getNotationName() const;

  DOM_Node		getFirstChild() const;
  DOM_Node      getLastChild() const;
  DOM_NodeList  getChildNodes() const;
  bool          hasChildNodes() const;
  DOM_Node		getPreviousSibling() const;
  DOM_Node		getNextSibling() const;
  //@}

protected:
    DOM_Entity(EntityImpl *impl);

    friend class DOM_Document;
};

XERCES_CPP_NAMESPACE_END

#endif

