#ifndef DOMLocator_HEADER_GUARD_
#define DOMLocator_HEADER_GUARD_

/*
 * Copyright 2002,2004 The Apache Software Foundation.
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
 * $Id: DOMLocator.hpp 191054 2005-06-17 02:56:35Z jberry $
 */

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN


class DOMNode;


/**
  * DOMLocator is an interface that describes a location. (e.g. where an error
  * occured).
  *
  * @see DOMError#DOMError
  * @since DOM Level 3
  */

class CDOM_EXPORT DOMLocator
{
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMLocator() {};
    //@}
    
private:    
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMLocator(const DOMLocator &);
    DOMLocator & operator = (const DOMLocator &);
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
    virtual ~DOMLocator() {};
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMLocator interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /**
     * Get the line number where the error occured. The value is -1 if there is
     * no line number available.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see #setLineNumber
     * @since DOM Level 3
     */
    virtual XMLSSize_t getLineNumber() const = 0;

    /**
     * Get the column number where the error occured. The value is -1 if there
     * is no column number available.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see #setColumnNumber
     * @since DOM Level 3
     */
    virtual XMLSSize_t getColumnNumber() const = 0;

    /**
     * Get the byte or character offset into the input source, if we're parsing
     * a file or a byte stream then this will be the byte offset into that
     * stream, but if a character media is parsed then the offset will be the
     * character offset. The value is -1 if there is no offset available.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see #setOffset
     * @since DOM Level 3
     */
    virtual XMLSSize_t getOffset() const = 0;

    /**
     * Get the DOMNode where the error occured, or <code>null</code> if there
     * is no node available.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see #setErrorNode
     * @since DOM Level 3
     */
    virtual DOMNode* getErrorNode() const = 0;

    /**
     * Get the URI where the error occured, or <code>null</code> if there is no
     * URI available.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see #setURI
     * @since DOM Level 3
     */
    virtual const XMLCh* getURI() const = 0;

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    /**
     * Set the line number of the error
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param lineNumber the line number to set
     *
     * @see #getLinNumner
     * @since DOM Level 3
     */
    virtual void setLineNumber(const XMLSSize_t lineNumber) = 0;

    /**
     * Set the column number of the error
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param columnNumber the column number to set.
     *
     * @see #getColumnNumner
     * @since DOM Level 3
     */
    virtual void setColumnNumber(const XMLSSize_t columnNumber) = 0;

    /**
     * Set the byte/character offset.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param offset the byte/characte offset to set.
     *
     * @see #getOffset
     * @since DOM Level 3
     */
    virtual void setOffset(const XMLSSize_t offset) = 0;

    /**
     * Set the DOMNode where the error occured
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param errorNode the DOMNode to set
     *
     * @see #getErrorNode
     * @since DOM Level 3
     */
    virtual void setErrorNode(DOMNode* const errorNode) = 0;

    /**
     * Set the URI where the error occured
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param uri the URI to set.
     *
     * @see #getURI
     * @since DOM Level 3
     */
    virtual void setURI(const XMLCh* const uri) = 0;

    //@}
};

XERCES_CPP_NAMESPACE_END

#endif
