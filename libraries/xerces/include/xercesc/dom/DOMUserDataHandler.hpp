#ifndef DOMUserDataHandler_HEADER_GUARD_
#define DOMUserDataHandler_HEADER_GUARD_

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
 * $Id: DOMUserDataHandler.hpp 176026 2004-09-08 13:57:07Z peiyongz $
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

