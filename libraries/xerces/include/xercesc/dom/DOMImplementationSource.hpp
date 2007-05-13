#ifndef DOMImplementationSource_HEADER_GUARD_
#define DOMImplementationSource_HEADER_GUARD_

/*
 * Copyright 2002-2004 The Apache Software Foundation.
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
 * $Id: DOMImplementationSource.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

/**
  * This interface permits a DOM implementer to supply one or more
  * implementations, based upon requested features. Each implemented
  * <code>DOMImplementationSource</code> object is listed in the
  * binding-specific list of available sources so that its
  * <code>DOMImplementation</code> objects are made available.
  * <p>See also the <a href='http://www.w3.org/TR/2002/WD-DOM-Level-3-Core-20020409'>Document Object Model (DOM) Level 3 Core Specification</a>.
  *
  * @since DOM Level 3
  */
#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN


class DOMImplementation;

class CDOM_EXPORT DOMImplementationSource
{
protected :
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMImplementationSource() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMImplementationSource(const DOMImplementationSource &);
    DOMImplementationSource & operator = (const DOMImplementationSource &);
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
    virtual ~DOMImplementationSource() {};
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMImplementationSource interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{
    /**
     * A method to request a DOM implementation.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param features A string that specifies which features are required.
     *   This is a space separated list in which each feature is specified
     *   by its name optionally followed by a space and a version number.
     *   This is something like: "XML 1.0 Traversal 2.0"
     * @return An implementation that has the desired features, or
     *   <code>null</code> if this source has none.
     * @since DOM Level 3
     */
    virtual DOMImplementation* getDOMImplementation(const XMLCh* features) const = 0;
    //@}

};

XERCES_CPP_NAMESPACE_END

#endif
