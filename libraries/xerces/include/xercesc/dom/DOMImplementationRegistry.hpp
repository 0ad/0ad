#ifndef DOMImplementationRegistry_HEADER_GUARD_
#define DOMImplementationRegistry_HEADER_GUARD_

/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002-2003 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 2002, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: DOMImplementationRegistry.hpp,v 1.5 2004/01/29 11:44:26 cargilld Exp $
 */

/**
  * This class holds the list of registered DOMImplementations.  Implementation
  * or application can register DOMImplementationSource to the registry, and
  * then can query DOMImplementation based on a list of requested features.
  *
  * <p>This provides an application with an implementation independent starting
  * point.
  *
  * @see DOMImplementation
  * @see DOMImplementationSource
  * @since DOM Level 3
  */

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN


class DOMImplementation;
class DOMImplementationSource;

class CDOM_EXPORT DOMImplementationRegistry
{
public:
    // -----------------------------------------------------------------------
    //  Static DOMImplementationRegistry interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{
    /**
     * Return the first registered implementation that has the desired features,
     * or null if none is found.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param features A string that specifies which features are required.
     *                 This is a space separated list in which each feature is
     *                 specified by its name optionally followed by a space
     *                 and a version number.
     *                 This is something like: "XML 1.0 Traversal 2.0"
     * @return An implementation that has the desired features, or
     *   <code>null</code> if this source has none.
     * @since DOM Level 3
     */
    static DOMImplementation* getDOMImplementation(const XMLCh* features);

    /**
     * Register an implementation.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param source   A DOMImplementation Source object to be added to the registry.
     *                 The registry does NOT adopt the source object.  Users still own it.
     * @since DOM Level 3
     */
    static void addSource(DOMImplementationSource* source);
    //@}

private:
    DOMImplementationRegistry();
};

XERCES_CPP_NAMESPACE_END

#endif
