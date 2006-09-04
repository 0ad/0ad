#ifndef DOMXPathNSResolver_HEADER_GUARD_
#define DOMXPathNSResolver_HEADER_GUARD_

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
 * The <code>DOMXPathNSResolver</code> interface permit prefix strings
 * in the expression to be properly bound to namespaceURI strings. 
 * <code>DOMXPathEvaluator</code> can construct an implementation of 
 * <code>DOMXPathNSResolver</code> from a node, or the interface may be
 * implemented by any application.
 * @since DOM Level 3
 */
class CDOM_EXPORT DOMXPathNSResolver
{

protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMXPathNSResolver() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMXPathNSResolver(const DOMXPathNSResolver &);
    DOMXPathNSResolver& operator = (const  DOMXPathNSResolver&);
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
    virtual ~DOMXPathNSResolver() {};
    //@}

    // -----------------------------------------------------------------------
    // Virtual DOMDocument interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{

    /** Look up the namespace URI associated to the given namespace prefix. 
     * The XPath evaluator must never call this with a null or empty argument, 
     * because the result of doing this is undefined.
     * @param prefix of type XMLCh - The prefix to look for.
     * @return the associated namespace URI or null if none is found.
     */
    virtual const XMLCh*          lookupNamespaceURI(const XMLCh* prefix) const = 0;
    //@}


    // -----------------------------------------------------------------------
    // Non-standard extension
    // -----------------------------------------------------------------------
    /** @name Non-standard extension */
    //@{

    /**
     * Non-standard extension
     *
     * XPath2 implementations require a reverse lookup in the static context.
     * Look up the prefix associated with the namespace URI
     * The XPath evaluator must never call this with a null or empty argument, 
     * because the result of doing this is undefined.
     * @param URI of type XMLCh - The namespace to look for.
     * @return the associated prefix or null if none is found.
     */
    virtual const XMLCh*          lookupPrefix(const XMLCh* URI) const = 0;


    //@}
};

XERCES_CPP_NAMESPACE_END

#endif

