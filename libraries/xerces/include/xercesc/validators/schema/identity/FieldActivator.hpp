/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2003 The Apache Software Foundation.  All rights
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
 * $Id: FieldActivator.hpp,v 1.6 2003/12/16 18:41:15 knoaman Exp $
 */

#if !defined(FIELDACTIVATOR_HPP)
#define FIELDACTIVATOR_HPP

/**
  * This class is responsible for activating fields within a specific scope;
  * the caller merely requests the fields to be activated.
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/ValueHashTableOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class IdentityConstraint;
class XPathMatcher;
class ValueStoreCache;
class IC_Field;
class XPathMatcherStack;


class VALIDATORS_EXPORT FieldActivator : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    FieldActivator(ValueStoreCache* const valueStoreCache,
                   XPathMatcherStack* const matcherStack,
                   MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	FieldActivator(const FieldActivator& other);
	~FieldActivator();

    // -----------------------------------------------------------------------
    //  Operator methods
    // -----------------------------------------------------------------------
    FieldActivator& operator =(const FieldActivator& other);

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    bool getMayMatch(IC_Field* const field);

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setValueStoreCache(ValueStoreCache* const other);
    void setMatcherStack(XPathMatcherStack* const matcherStack);
    void setMayMatch(IC_Field* const field, bool value);

	// -----------------------------------------------------------------------
    //  Activation methods
    // -----------------------------------------------------------------------
    /**
      * Start the value scope for the specified identity constraint. This
      * method is called when the selector matches in order to initialize
      * the value store.
      */
    void startValueScopeFor(const IdentityConstraint* const ic, const int initialDepth);

    /**
      * Request to activate the specified field. This method returns the
      * matcher for the field.
      */
    XPathMatcher* activateField(IC_Field* const field, const int initialDepth);

    /**
      * Ends the value scope for the specified identity constraint.
      */
    void endValueScopeFor(const IdentityConstraint* const ic, const int initialDepth);

private:
    // -----------------------------------------------------------------------
    //  Data
    // -----------------------------------------------------------------------
    ValueStoreCache*        fValueStoreCache;
    XPathMatcherStack*      fMatcherStack;
    ValueHashTableOf<bool>* fMayMatch;
    MemoryManager*          fMemoryManager;
};


// ---------------------------------------------------------------------------
//  FieldActivator: Getter methods
// ---------------------------------------------------------------------------
inline bool FieldActivator::getMayMatch(IC_Field* const field) {

    return fMayMatch->get(field);
}

// ---------------------------------------------------------------------------
//  FieldActivator: Setter methods
// ---------------------------------------------------------------------------
inline void FieldActivator::setValueStoreCache(ValueStoreCache* const other) {

    fValueStoreCache = other;
}

inline void
FieldActivator::setMatcherStack(XPathMatcherStack* const matcherStack) {

    fMatcherStack = matcherStack;
}

inline void FieldActivator::setMayMatch(IC_Field* const field, bool value) {

    fMayMatch->put(field, value);
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file FieldActivator.hpp
  */

