#ifndef DOMException_HEADER_GUARD_
#define DOMException_HEADER_GUARD_

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
 * $Id: DOMException.hpp,v 1.6 2004/01/29 11:44:26 cargilld Exp $
 */

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN


/**
 * DOM operations only raise exceptions in "exceptional" circumstances, i.e.,
 * when an operation is impossible to perform (either for logical reasons,
 * because data is lost, or because the implementation has become unstable).
 * In general, DOM methods return specific error values in ordinary
 * processing situations, such as out-of-bound errors when using
 * <code>DOMNodeList</code>.
 * <p>Implementations should raise other exceptions under other circumstances.
 * For example, implementations should raise an implementation-dependent
 * exception if a <code>null</code> argument is passed.
 * <p>Some languages and object systems do not support the concept of
 * exceptions. For such systems, error conditions may be indicated using
 * native error reporting mechanisms. For some bindings, for example,
 * methods may return error codes similar to those listed in the
 * corresponding method descriptions.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113'>Document Object Model (DOM) Level 2 Core Specification</a>.
 * @since DOM Level 1
 */

class CDOM_EXPORT DOMException  {
public:
    // -----------------------------------------------------------------------
    //  Constructors
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{
    /**
      * Default constructor for DOMException.
      *
      */
    DOMException();

    /**
      * Constructor which takes an error code and a message.
      *
      * @param code The error code which indicates the exception
      * @param message The string containing the error message
      */
    DOMException(short code, const XMLCh *message);

    /**
      * Copy constructor.
      *
      * @param other The object to be copied.
      */
    DOMException(const DOMException &other);

    //@}

    // -----------------------------------------------------------------------
    //  Destructors
    // -----------------------------------------------------------------------
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOMException.
	  *
	  */
    virtual ~DOMException();
    //@}

public:
    // -----------------------------------------------------------------------
    //  Class Types
    // -----------------------------------------------------------------------
    /** @name Public Contants */
    //@{
    /**
     * ExceptionCode
     *
     * <p><code>INDEX_SIZE_ERR:</code>
     * If index or size is negative, or greater than the allowed value.</p>
     *
     * <p><code>DOMSTRING_SIZE_ERR:</code>
     * If the specified range of text does not fit into a DOMString.</p>
     *
     * <p><code>HIERARCHY_REQUEST_ERR:</code>
     * If any node is inserted somewhere it doesn't belong.</p>
     *
     * <p><code>WRONG_DOCUMENT_ERR:</code>
     * If a node is used in a different document than the one that created it
     * (that doesn't support it).</p>
     *
     * <p><code>INVALID_CHARACTER_ERR:</code>
     * If an invalid or illegal character is specified, such as in a name. See
     * production 2 in the XML specification for the definition of a legal
     * character, and production 5 for the definition of a legal name
     * character.</p>
     *
     * <p><code>NO_DATA_ALLOWED_ERR:</code>
     * If data is specified for a node which does not support data.</p>
     *
     * <p><code>NO_MODIFICATION_ALLOWED_ERR:</code>
     * If an attempt is made to modify an object where modifications are not
     * allowed.</p>
     *
     * <p><code>NOT_FOUND_ERR:</code>
     * If an attempt is made to reference a node in a context where it does
     * not exist.</p>
     *
     * <p><code>NOT_SUPPORTED_ERR:</code>
     * If the implementation does not support the requested type of object or
     * operation.</p>
     *
     * <p><code>INUSE_ATTRIBUTE_ERR:</code>
     * If an attempt is made to add an attribute that is already in use
     * elsewhere.</p>
     *
     * The above are since DOM Level 1
     * @since DOM Level 1
     *
     * <p><code>INVALID_STATE_ERR:</code>
     * If an attempt is made to use an object that is not, or is no longer,
     * usable.</p>
     *
     * <p><code>SYNTAX_ERR:</code>
     * If an invalid or illegal string is specified.</p>
     *
     * <p><code>INVALID_MODIFICATION_ERR:</code>
     * If an attempt is made to modify the type of the underlying object.</p>
     *
     * <p><code>NAMESPACE_ERR:</code>
     * If an attempt is made to create or change an object in a way which is
     * incorrect with regard to namespaces.</p>
     *
     * <p><code>INVALID_ACCESS_ERR:</code>
     * If a parameter or an operation is not supported by the underlying
     * object.
     *
     * The above are since DOM Level 2
     * @since DOM Level 2
     *
     * <p><code>VALIDATION_ERR:</code>
     * If a call to a method such as <code>insertBefore</code> or
     * <code>removeChild</code> would make the <code>Node</code> invalid
     * with respect to "partial validity", this exception would be raised
     * and the operation would not be done.
     *
     * The above is since DOM Level 2
     * @since DOM Level 3
     */
    enum ExceptionCode {
         INDEX_SIZE_ERR       = 1,
         DOMSTRING_SIZE_ERR   = 2,
         HIERARCHY_REQUEST_ERR = 3,
         WRONG_DOCUMENT_ERR   = 4,
         INVALID_CHARACTER_ERR = 5,
         NO_DATA_ALLOWED_ERR  = 6,
         NO_MODIFICATION_ALLOWED_ERR = 7,
         NOT_FOUND_ERR        = 8,
         NOT_SUPPORTED_ERR    = 9,
         INUSE_ATTRIBUTE_ERR  = 10,
         INVALID_STATE_ERR    = 11,
         SYNTAX_ERR     = 12,
         INVALID_MODIFICATION_ERR    = 13,
         NAMESPACE_ERR     = 14,
         INVALID_ACCESS_ERR   = 15,
         VALIDATION_ERR       = 16
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
    DOMException & operator = (const DOMException &);
};

XERCES_CPP_NAMESPACE_END

#endif

