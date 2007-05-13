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

/*
 * $Id: IC_Selector.hpp 176026 2004-09-08 13:57:07Z peiyongz $
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

