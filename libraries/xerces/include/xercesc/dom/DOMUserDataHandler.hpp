#ifndef DOMUserDataHandler_HEADER_GUARD_
#define DOMUserDataHandler_HEADER_GUARD_

/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002 The Apache Software Foundation.  All rights
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
 * $Id: DOMUserDataHandler.hpp,v 1.6 2003/03/07 19:59:09 tng Exp $
 */

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/dom/DOMNode.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * When associating an object to a key on a node using <code>setUserData</code>
 *  the application can provide a handler that gets called when the node the
 * object is associated to is being cloned or imported. This can be used by
 * the application to implement various behaviors regarding the data it
 * associates to the DOM nodes. This interface defines that handler.
 *
 * <p><b>"Experimental - subject to change"</b></p>
 *
 * <p>See also the <a href='http://www.w3.org/2001/07/WD-DOM-Level-3-Core-20010726'>Document Object Model (DOM) Level 3 Core Specification</a>.
 * @since DOM Level 3
 */
class CDOM_EXPORT DOMUserDataHandler {
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMUserDataHandler() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMUserDataHandler(const DOMUserDataHandler &);
    DOMUserDataHandler & operator = (const DOMUserDataHandler &);
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
    virtual ~DOMUserDataHandler() {};
    //@}

    // -----------------------------------------------------------------------
    //  Class Types
    // -----------------------------------------------------------------------
    /** @name Public Constants */
    //@{
    /**
     * Operation Type
     *
     * <p><code>NODE_CLONED:</code>
     * The node is cloned.</p>
     *
     * <p><code>NODE_IMPORTED</code>
     * The node is imported.</p>
     *
     * <p><code>NODE_DELETED</code>
     * The node is deleted.</p>
     *
     * <p><code>NODE_RENAMED</code>
     * The node is renamed.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @since DOM Level 3
     */
    enum DOMOperationType {
        NODE_CLONED               = 1,
        NODE_IMPORTED             = 2,
        NODE_DELETED              = 3,
        NODE_RENAMED              = 4
    };
    //@}


    // -----------------------------------------------------------------------
    //  Virtual DOMUserDataHandler interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{
    /**
     * This method is called whenever the node for which this handler is
     * registered is imported or cloned.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param operation Specifies the type of operation that is being
     *   performed on the node.
     * @param key Specifies the key for which this handler is being called.
     * @param data Specifies the data for which this handler is being called.
     * @param src Specifies the node being cloned, imported, or renamed.
     * @param dst Specifies the node newly created.
     * @since DOM Level 3
     */
    virtual void handle(DOMOperationType operation,
                        const XMLCh* const key,
                        void* data,
                        const DOMNode* src,
                        const DOMNode* dst) = 0;

    //@}

};

XERCES_CPP_NAMESPACE_END

#endif

