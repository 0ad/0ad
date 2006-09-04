/*
 * Copyright 2003,2004 The Apache Software Foundation.
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

#if !defined(DOMTYPEINFO_HPP)
#define DOMTYPEINFO_HPP

//------------------------------------------------------------------------------------
//  Includes
//------------------------------------------------------------------------------------
#include <xercesc/util/XMLString.hpp>


XERCES_CPP_NAMESPACE_BEGIN

/**
  * The <code>DOMTypeInfo</code> interface represent a type used by 
  * <code>DOMElement</code> or <code>DOMAttr</code> nodes, specified in the 
  * schemas associated with the document. The type is a pair of a namespace URI
  * and name properties, and depends on the document's schema.
  */
class CDOM_EXPORT DOMTypeInfo
{
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMTypeInfo() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMTypeInfo(const DOMTypeInfo &);
    DOMTypeInfo & operator = (const DOMTypeInfo &);
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
    virtual ~DOMTypeInfo() {};
    //@}

    //@{
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /**
     * Returns The name of a type declared for the associated <code>DOMElement</code> 
     * or <code>DOMAttr</code>, or null if undeclared.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @return The name of a type declared for the associated <code>DOMElement</code> 
     * or <code>DOMAttribute</code>, or null if undeclared.
     * @since DOM level 3
     */
    virtual const XMLCh* getName() const = 0;

    /**
     * The namespace of the type declared for the associated <code>DOMElement</code> 
     * or <code>DOMAttr</code> or null if the <code>DOMElement</code> does not have 
     * declaration or if no namespace information is available.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @return The namespace of the type declared for the associated <code>DOMElement</code> 
     * or <code>DOMAttr</code> or null if the <code>DOMElement</code> does not have 
     * declaration or if no namespace information is available.
     * @since DOM level 3
     */
    virtual const XMLCh* getNamespace() const = 0;
    //@}
};

XERCES_CPP_NAMESPACE_END

#endif

/**
 * End of file DOMTypeInfo.hpp
 */
