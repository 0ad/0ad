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
 * $Id: IC_Selector.hpp,v 1.8 2003/10/14 15:24:23 peiyongz Exp $
 */

#if !defined(IC_SELECTOR_HPP)
#define IC_SELECTOR_HPP


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/schema/identity/XPathMatcher.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class FieldActivator;


class VALIDATORS_EXPORT IC_Selector : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    IC_Selector(XercesXPath* const xpath,
                IdentityConstraint* const identityConstraint);
	~IC_Selector();

    // -----------------------------------------------------------------------
    //  operators
    // -----------------------------------------------------------------------
    bool operator== (const IC_Selector& other) const;
    bool operator!= (const IC_Selector& other) const;

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    XercesXPath* getXPath() const { return fXPath; }
    IdentityConstraint* getIdentityConstraint() const { return fIdentityConstraint; }

	// -----------------------------------------------------------------------
    //  Factory methods
    // -----------------------------------------------------------------------
    XPathMatcher* createMatcher(FieldActivator* const fieldActivator,
                                const int initialDepth,
                                MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(IC_Selector)

    IC_Selector(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    IC_Selector(const IC_Selector& other);
    IC_Selector& operator= (const IC_Selector& other);

    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    XercesXPath*        fXPath;
    IdentityConstraint* fIdentityConstraint;
};


class VALIDATORS_EXPORT SelectorMatcher : public XPathMatcher
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    ~SelectorMatcher() {}

    int getInitialDepth() const { return fInitialDepth; }

    // -----------------------------------------------------------------------
    //  XMLDocumentHandler methods
    // -----------------------------------------------------------------------
    void startDocumentFragment();
    void startElement(const XMLElementDecl& elemDecl,
                      const unsigned int urlId,
                      const XMLCh* const elemPrefix,
		              const RefVectorOf<XMLAttr>& attrList,
                      const unsigned int attrCount);
    void endElement(const XMLElementDecl& elemDecl,
                    const XMLCh* const elemContent);

private:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    SelectorMatcher(XercesXPath* const anXPath,
                    IC_Selector* const selector,
                    FieldActivator* const fieldActivator,
                    const int initialDepth,
                    MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    SelectorMatcher(const SelectorMatcher& other);
    SelectorMatcher& operator= (const SelectorMatcher& other);

    // -----------------------------------------------------------------------
    //  Friends
    // -----------------------------------------------------------------------
    friend class IC_Selector;

    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    int             fInitialDepth;
    int             fElementDepth;
    int             fMatchedDepth;
    IC_Selector*    fSelector;
    FieldActivator* fFieldActivator;
};

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file IC_Selector.hpp
  */

