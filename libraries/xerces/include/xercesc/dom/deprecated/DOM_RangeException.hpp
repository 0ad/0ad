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
 * $Id: DOM_RangeException.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_RangeException_HEADER_GUARD_
#define DOM_RangeException_HEADER_GUARD_

#include "DOM_DOMException.hpp"

XERCES_CPP_NAMESPACE_BEGIN


/**
  * Encapsulate range related DOM error or warning. DOM level 2 implementation.
  *
  * <p> The DOM will create and throw an instance of DOM_RangeException
  * when an error condition in range is detected.  Exceptions can occur
  * when an application directly manipulates the range elements in DOM document
  * tree that is produced by the parser.
  *
  * <p>Unlike the other classes in the C++ DOM API, DOM_RangeException
  * is NOT a reference to an underlying implementation class, and
  * does not provide automatic memory management.  Code that catches
  * a DOM Range exception is responsible for deleting it, or otherwise
  * arranging for its disposal.
  *
  */
class DEPRECATED_DOM_EXPORT DOM_RangeException  : public DOM_DOMException {
public:
    /** @name Enumerators for DOM Range Exceptions */
    //@{
        enum RangeExceptionCode {
                BAD_BOUNDARYPOINTS_ERR  = 1,
                INVALID_NODE_TYPE_ERR   = 2
        };
    //@}
public:
    /** @name Constructors and assignment operator */
    //@{
    /**
      * Default constructor for DOM_RangeException.
      *
      */
    DOM_RangeException();

    /**
      * Constructor which takes an error code and a message.
      *
      * @param code The error code which indicates the exception
      * @param message The string containing the error message
      */
    DOM_RangeException(RangeExceptionCode code, const DOMString &message);

    /**
      * Copy constructor.
      *
      * @param other The object to be copied.
      */
    DOM_RangeException(const DOM_RangeException &other);

    //@}
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOM_RangeException.  Applications are responsible
      * for deleting DOM_RangeException objects that they catch after they
      * have completed their exception processing.
	  *
	  */
    virtual ~DOM_RangeException();
    //@}

    /** @name Public variables. */
     //@{
	 /**
	  * A code value, from the set defined by the RangeExceptionCode enum,
      * indicating the type of error that occured.
	  */
   RangeExceptionCode   code;

    //@}

};

XERCES_CPP_NAMESPACE_END

#endif

