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
 * $Id: DOM_RangeException.hpp,v 1.3 2002/11/04 15:04:44 tng Exp $
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
class CDOM_EXPORT DOM_RangeException  : public DOM_DOMException {
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

