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
 * $Id: DOM_CharacterData.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_CharacterData_HEADER_GUARD_
#define DOM_CharacterData_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_Node.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class CharacterDataImpl;

/**
 * The <code>DOM_CharacterData</code> interface extends Node with a set  of
 * methods for accessing character data in the DOM.
 *
 * For clarity this set is defined here rather than on each class that uses
 * these methods. No DOM objects correspond directly to
 * <code>CharacterData</code>, though <code>Text</code> and others do inherit
 * the interface from it. All <code>offset</code>s in this interface start
 * from 0, and index in terms of Unicode 16 bit storage units.
 */
class DEPRECATED_DOM_EXPORT DOM_CharacterData: public DOM_Node {

private:

public:
  /** @name Constructors and assignment operator */
  //@{
  /**
    * Default constructor for DOM_CharacterData.  While there can be
    * no actual DOM nodes of type CharacterData, the C++ objects
    * function more like reference variables, and instances of
    * <code>DOM_CharacterData</code> can exist.  They will be null when created
    * by this constructor, and can then be assigned to refer to Text
    * or CDATASection nodes.
    */
    DOM_CharacterData();

  /**
    * Copy constructor
    *
    * @param other The object to be copied
    */
    DOM_CharacterData(const DOM_CharacterData &other);
  /**
    * Assignment operator
    *
    * @param other The object to be copied
    */
    DOM_CharacterData & operator = (const DOM_CharacterData &other);

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
    DOM_CharacterData & operator = (const DOM_NullPtr *val);

    //@}
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOM_CharacterData.  The object being destroyed
      * is the reference to the Character Data node, not the character
      * data itself.
	  */
    ~DOM_CharacterData();


    //@}

    /** @name Getter functions. */
    //@{
  /**
   * Returns the character data of the node that implements this interface.
   *
   * The DOM implementation may not put arbitrary limits on the amount of data that
   * may be stored in a  <code>CharacterData</code> node. However,
   * implementation limits may  mean that the entirety of a node's data may
   * not fit into a single <code>DOMString</code>. In such cases, the user
   * may call <code>substringData</code> to retrieve the data in
   * appropriately sized pieces.
   * @exception DOMException
   *   NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
   * @exception DOMException
   *   DOMSTRING_SIZE_ERR: Raised when it would return more characters than
   *   fit in a <code>DOMString</code> variable on the implementation
   *   platform.
   */
  DOMString          getData() const;
  /**
   * Returns the number of characters that are available through <code>data</code> and
   * the <code>substringData</code> method below.
   *
   * This may have the value
   * zero, i.e., <code>CharacterData</code> nodes may be empty.
   */
  unsigned int       getLength() const;
  /**
   * Extracts a range of data from the node.
   *
   * @param offset Start offset of substring to extract.
   * @param count The number of characters to extract.
   * @return The specified substring. If the sum of <code>offset</code> and
   *   <code>count</code> exceeds the <code>length</code>, then all
   *   characters to the end of the data are returned.
   * @exception DOMException
   *   INDEX_SIZE_ERR: Raised if the specified offset is negative or greater
   *   than the number of characters in <code>data</code>, or if the
   *   specified <code>count</code> is negative.
   *   <br>DOMSTRING_SIZE_ERR: Raised if the specified range of text does not
   *   fit into a <code>DOMString</code>.
   */
  DOMString          substringData(unsigned int offset,
                                   unsigned int count) const;
    //@}
    /** @name Functions that set or change data. */
    //@{
  /**
   * Append the string to the end of the character data of the node.
   *
   * Upon success, <code>data</code> provides access to the concatenation of
   * <code>data</code> and the <code>DOMString</code> specified.
   * @param arg The <code>DOMString</code> to append.
   * @exception DOMException
   *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
   */
  void               appendData(const DOMString &arg);
  /**
   * Insert a string at the specified character offset.
   *
   * @param offset The character offset at which to insert.
   * @param arg The <code>DOMString</code> to insert.
   * @exception DOMException
   *   INDEX_SIZE_ERR: Raised if the specified offset is negative or greater
   *   than the number of characters in <code>data</code>.
   *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
   */
  void               insertData(unsigned int offset, const  DOMString &arg);
  /**
   * Remove a range of characters from the node.
   *
   * Upon success,
   * <code>data</code> and <code>length</code> reflect the change.
   * @param offset The offset from which to remove characters.
   * @param count The number of characters to delete. If the sum of
   *   <code>offset</code> and <code>count</code> exceeds <code>length</code>
   *   then all characters from <code>offset</code> to the end of the data
   *   are deleted.
   * @exception DOMException
   *   INDEX_SIZE_ERR: Raised if the specified offset is negative or greater
   *   than the number of characters in <code>data</code>, or if the
   *   specified <code>count</code> is negative.
   *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
   */
  void               deleteData(unsigned int offset,
                                unsigned int count);
  /**
   * Replace the characters starting at the specified character offset with
   * the specified string.
   *
   * @param offset The offset from which to start replacing.
   * @param count The number of characters to replace. If the sum of
   *   <code>offset</code> and <code>count</code> exceeds <code>length</code>
   *   , then all characters to the end of the data are replaced (i.e., the
   *   effect is the same as a <code>remove</code> method call with the same
   *   range, followed by an <code>append</code> method invocation).
   * @param arg The <code>DOMString</code> with which the range must be
   *   replaced.
   * @exception DOMException
   *   INDEX_SIZE_ERR: Raised if the specified offset is negative or greater
   *   than the number of characters in <code>data</code>, or if the
   *   specified <code>count</code> is negative.
   *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
   */
  void               replaceData(unsigned int offset,
                                 unsigned int count,
                                 const DOMString &arg);

  /**
   * Sets the character data of the node that implements this interface.
   *
   * @param data The <code>DOMString</code> to set.
   */
  void               setData(const DOMString &data);
  //@}

protected:
    DOM_CharacterData(CharacterDataImpl *impl);

};

XERCES_CPP_NAMESPACE_END

#endif


