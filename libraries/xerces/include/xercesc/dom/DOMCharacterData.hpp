#ifndef DOMCharacterData_HEADER_GUARD_
#define DOMCharacterData_HEADER_GUARD_

/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2002 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 2001, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: DOMCharacterData.hpp,v 1.8 2003/03/07 19:59:01 tng Exp $
 */

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/dom/DOMNode.hpp>

XERCES_CPP_NAMESPACE_BEGIN


/**
 * The <code>DOMCharacterData</code> interface extends DOMNode with a set of
 * attributes and methods for accessing character data in the DOM. For
 * clarity this set is defined here rather than on each object that uses
 * these attributes and methods. No DOM objects correspond directly to
 * <code>DOMCharacterData</code>, though <code>DOMText</code> and others do
 * inherit the interface from it. All <code>offsets</code> in this interface
 * start from <code>0</code>.
 * <p>As explained in the DOM spec, text strings in
 * the DOM are represented in UTF-16, i.e. as a sequence of 16-bit units. In
 * the following, the term 16-bit units is used whenever necessary to
 * indicate that indexing on DOMCharacterData is done in 16-bit units.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113'>Document Object Model (DOM) Level 2 Core Specification</a>.
 * @since DOM Level 1
 */
class CDOM_EXPORT DOMCharacterData: public DOMNode {
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMCharacterData() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMCharacterData(const DOMCharacterData &);
    DOMCharacterData & operator = (const DOMCharacterData &);
    //@}

public:
    // -----------------------------------------------------------------------
    //  All constructors are hidden, just the destructor is available
    // -----------------------------------------------------------------------
    /** @name Destructor */
    //@{
    /**
     * Destructor
     *
     */
    virtual ~DOMCharacterData() {};
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMCharacterData interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 1 */
    //@{
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /**
     * Returns the character data of the node that implements this interface.
     *
     * The DOM implementation may not put arbitrary limits on the amount of data that
     * may be stored in a  <code>DOMCharacterData</code> node. However,
     * implementation limits may  mean that the entirety of a node's data may
     * not fit into a single <code>XMLCh* String</code>. In such cases, the user
     * may call <code>substringData</code> to retrieve the data in
     * appropriately sized pieces.
     * @exception DOMException
     *   NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
     * @since DOM Level 1
     */
    virtual const XMLCh *     getData() const = 0;

    /**
     * Returns the number of characters that are available through <code>data</code> and
     * the <code>substringData</code> method below.
     *
     * This may have the value
     * zero, i.e., <code>CharacterData</code> nodes may be empty.
     * @since DOM Level 1
     */
    virtual XMLSize_t       getLength() const = 0;

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
     * @since DOM Level 1
     */
    virtual const XMLCh *     substringData(XMLSize_t offset,
                                     XMLSize_t count) const = 0;

    // -----------------------------------------------------------------------
    //  String methods
    // -----------------------------------------------------------------------
    /**
     * Append the string to the end of the character data of the node.
     *
     * Upon success, <code>data</code> provides access to the concatenation of
     * <code>data</code> and the <code>XMLCh* String</code> specified.
     * @param arg The <code>XMLCh* String</code> to append.
     * @exception DOMException
     *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     * @since DOM Level 1
     */
    virtual void               appendData(const XMLCh *arg) = 0;

    /**
     * Insert a string at the specified character offset.
     *
     * @param offset The character offset at which to insert.
     * @param arg The <code>XMLCh* String</code> to insert.
     * @exception DOMException
     *   INDEX_SIZE_ERR: Raised if the specified offset is negative or greater
     *   than the number of characters in <code>data</code>.
     *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     * @since DOM Level 1
     */
    virtual void               insertData(XMLSize_t offset, const  XMLCh *arg) = 0;

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
     * @since DOM Level 1
     */
    virtual void               deleteData(XMLSize_t offset,
                                  XMLSize_t count) = 0;

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
     * @param arg The <code>XMLCh* String</code> with which the range must be
     *   replaced.
     * @exception DOMException
     *   INDEX_SIZE_ERR: Raised if the specified offset is negative or greater
     *   than the number of characters in <code>data</code>, or if the
     *   specified <code>count</code> is negative.
     *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     * @since DOM Level 1
     */
    virtual void               replaceData(XMLSize_t offset,
                                   XMLSize_t count,
                                   const XMLCh *arg) = 0;

    /**
     * Sets the character data of the node that implements this interface.
     *
     * @param data The <code>XMLCh* String</code> to set.
     * @since DOM Level 1
     */
    virtual void               setData(const XMLCh *data) = 0;
    //@}

};

XERCES_CPP_NAMESPACE_END

#endif


