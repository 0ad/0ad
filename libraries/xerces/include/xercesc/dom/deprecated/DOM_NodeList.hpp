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
 * $Id: DOM_NodeList.hpp,v 1.3 2002/11/04 15:04:44 tng Exp $
 */

#ifndef DOM_NodeList_HEADER_GUARD_
#define DOM_NodeList_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_Node.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class NodeListImpl;

/**
 * The <code>NodeList</code> interface provides the abstraction of an ordered
 * collection of nodes.  NodeLists are created by DOM_Document::getElementsByTagName(),
 * DOM_Node::getChildNodes(),
 *
 * <p>The items in the <code>NodeList</code> are accessible via an integral
 * index, starting from 0.
 *
 * NodeLists are "live", in that any changes to the document tree are immediately
 * reflected in any NodeLists that may have been created for that tree.
 */

class  CDOM_EXPORT DOM_NodeList {
private:
    NodeListImpl *fImpl;

public:
    /** @name Constructors and assignment operator */
    //@{
    /**
      * Default constructor for DOM_NodeList.  The resulting object does not
      * refer to an actual NodeList; it will compare == to 0, and is similar
      * to a null object reference variable in Java.  It may subsequently be
      * assigned to refer to an actual NodeList.
      *
      */
    DOM_NodeList();

    /**
      * Copy constructor.
      *
      * @param other The object to be copied.
      */
    DOM_NodeList(const DOM_NodeList &other);

    /**
      * Assignment operator.
      *
      * @param other The object to be copied.
      */
    DOM_NodeList & operator = (const DOM_NodeList &other);

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
    DOM_NodeList & operator = (const DOM_NullPtr *val);

    //@}
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOM_NodeList.  The object being destroyed is the reference
      * object, not the underlying NodeList node itself.
	  *
      * <p>Like most other DOM types in this implementation, memory management
      * of Node Lists is automatic.  Instances of DOM_NodeList function
      * as references to an underlying heap based implementation object,
      * and should never be explicitly new-ed or deleted in application code, but
      * should appear only as local variables or function parameters.
	  */
    ~DOM_NodeList();
    //@}

    /** @name Comparison operators. */
    //@{

    /**
      *  Equality operator.
      *  Compares whether two node list
      *  variables refer to the same underlying node list.  It does
      *  not compare the contents of the node lists themselves.
      *
      *  @param other The value to be compared
      *  @return Returns true if node list refers to same underlying node list
      */
    bool operator == (const DOM_NodeList &other) const;

    /**
     *  Use this comparison operator to test whether a Node List reference
     *  is null.
     *
     *  @param nullPtr The value to be compared, which must be 0 or null.
     *  @return Returns true if node list reference is null
     */
    bool operator == (const DOM_NullPtr *nullPtr) const;

     /**
      *  Inequality operator.
      *  Compares whether two node list
      *  variables refer to the same underlying node list.  It does
      *  not compare the contents of the node lists themselves.
      *
      *  @param other The value to be compared
      *  @return Returns true if node list refers to a different underlying node list
      */
    bool operator != (const DOM_NodeList &other) const;

    /**
     *  Use this comparison operator to test whether a Node List reference
     *  is not null.
     *
     *  @param nullPtr The value to be compared, which must be 0 or null.
     *  @return Returns true if node list reference is not null
     */
    bool operator != (const DOM_NullPtr *nullPtr) const;
    //@}


    /** @name Get functions. */
    //@{
    /**
     * Returns the <code>index</code>th item in the collection.
     *
     * If <code>index</code> is greater than or equal to the number of nodes in
     * the list, this returns <code>null</code>.
     *
     * @param index Index into the collection.
     * @return The node at the <code>index</code>th position in the
     *   <code>NodeList</code>, or <code>null</code> if that is not a valid
     *   index.
     */
    DOM_Node  item(unsigned int index) const;

    /**
     * Returns the number of nodes in the list.
     *
     * The range of valid child node indices is 0 to <code>length-1</code> inclusive.
     */
    unsigned int getLength() const;
    //@}

protected:
    DOM_NodeList(NodeListImpl *impl);

    friend class DOM_Document;
    friend class DOM_Element;
    friend class DOM_Node;
    friend class DOM_Entity;

};

XERCES_CPP_NAMESPACE_END

#endif


