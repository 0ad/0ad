#ifndef DOMXPathException_HEADER_GUARD_
#define DOMXPathException_HEADER_GUARD_

/*
 * Copyright 2001-2004 The Apache Software Foundation.
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

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * @since DOM Level 3
 */
class CDOM_EXPORT DOMXPathException
{
public:
    // -----------------------------------------------------------------------
    //  Constructors
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{
    /**
      * Default constructor for DOMXPathException.
      *
      */
    DOMXPathException();

    /**
      * Constructor which takes an error code and a message.
      *
      * @param code The error code which indicates the exception
      * @param message The string containing the error message
      */
    DOMXPathException(short code, const XMLCh *message);

    /**
      * Copy constructor.
      *
      * @param other The object to be copied.
      */
    DOMXPathException(const DOMXPathException  &other);

    //@}

    // -----------------------------------------------------------------------
    //  Destructors
    // -----------------------------------------------------------------------
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOMXPathException.
	  *
	  */
    virtual ~DOMXPathException();
    //@}

public:

    //@{
    /**
     * ExceptionCode
     * INVALID_EXPRESSION_ERR If the expression has a syntax error or otherwise
     * is not a legal expression according to the rules of the specific 
     * <code>XPathEvaluator</code> or contains specialized extension functions
     * or variables not supported by this implementation.
     * TYPE_ERR If the expression cannot be converted to return the specified type.
     */
	enum ExceptionCode {
		INVALID_EXPRESSION_ERR = 51,
		TYPE_ERR = 52
	};
    //@}

    // -----------------------------------------------------------------------
    //  Class Types
    // -----------------------------------------------------------------------
    /** @name Public variables */
    //@{
	 /**
	  * A code value, from the set defined by the ExceptionCode enum,
     * indicating the type of error that occured.
     */
    ExceptionCode   code;

	 /**
	  * A string value.  Applications may use this field to hold an error
     *  message.  The field value is not set by the DOM implementation,
     *  meaning that the string will be empty when an exception is first
     *  thrown.
	  */
    const XMLCh *msg;
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    DOMXPathException& operator = (const DOMXPathException&);
};

XERCES_CPP_NAMESPACE_END

#endif
