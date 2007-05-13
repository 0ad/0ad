#ifndef DOMRangeException_HEADER_GUARD_
#define DOMRangeException_HEADER_GUARD_

/*
 * Copyright 2001-2002,2004 The Apache Software Foundation.
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
 * $Id: DOMRangeException.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#include <xercesc/dom/DOMException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * Range operations may throw a <code>DOMRangeException</code> as specified in
 * their method descriptions.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Traversal-Range-20001113'>Document Object Model (DOM) Level 2 Traversal and Range Specification</a>.
 * @since DOM Level 2
 */

class CDOM_EXPORT DOMRangeException  : public DOMException {
public:
    // -----------------------------------------------------------------------
    //  Class Types
    // -----------------------------------------------------------------------
    /** @name Public Contants */
    //@{
    /**
     * Enumerators for DOM Range Exceptions
     *
     * <p><code>BAD_BOUNDARYPOINTS_ERR:</code>
     * If the boundary-points of a Range do not meet specific requirements.</p>
     *
     * <p><code>INVALID_NODE_TYPE_ERR:</code>
     * If the container of an boundary-point of a Range is being set to either
     * a node of an invalid type or a node with an ancestor of an invalid
     * type.</p>
     *
     * @since DOM Level 2
     */
        enum RangeExceptionCode {
                BAD_BOUNDARYPOINTS_ERR  = 1,
                INVALID_NODE_TYPE_ERR   = 2
        };
    //@}

public:
    // -----------------------------------------------------------------------
    //  Constructors
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{
    /**
      * Default constructor for DOMRangeException.
      *
      */
    DOMRangeException();

    /**
      * Constructor which takes an error code and a message.
      *
      * @param code The error code which indicates the exception
      * @param message The string containing the error message
      * @param memoryManager  The memory manager used to (de)allocate memory
      */
    DOMRangeException(
                            RangeExceptionCode       code
                    , const XMLCh*                   message
                    ,       MemoryManager*     const memoryManager
                     );

    /**
      * Copy constructor.
      *
      * @param other The object to be copied.
      */
    DOMRangeException(const DOMRangeException &other);
    //@}

    // -----------------------------------------------------------------------
    //  Destructors
    // -----------------------------------------------------------------------
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOMRangeException.
	  *
	  */
    virtual ~DOMRangeException();
    //@}

public:
    // -----------------------------------------------------------------------
    //  Class Types
    // -----------------------------------------------------------------------
    /** @name Public variables */
    //@{
	 /**
	  * A code value, from the set defined by the RangeExceptionCode enum,
     * indicating the type of error that occured.
     *
     * @since DOM Level 2
	  */
    RangeExceptionCode   code;

    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    DOMRangeException & operator = (const DOMRangeException &);
};

XERCES_CPP_NAMESPACE_END

#endif

