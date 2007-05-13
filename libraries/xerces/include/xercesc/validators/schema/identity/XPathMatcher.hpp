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
 * $Id: XPathMatcher.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(XPATHMATCHER_HPP)
#define XPATHMATCHER_HPP


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/ValueStackOf.hpp>
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/framework/XMLBuffer.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class XMLElementDecl;
class XercesXPath;
class IdentityConstraint;
class DatatypeValidator;
class XMLStringPool;
class XercesLocationPath;
class XMLAttr;

class VALIDATORS_EXPORT XPathMatcher : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    XPathMatcher(XercesXPath* const xpath,
                 MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    XPathMatcher(XercesXPath* const xpath,
                 IdentityConstraint* const ic,
                 MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    virtual ~XPathMatcher();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    IdentityConstraint* getIdentityConstraint() const { return fIdentityConstraint; }
    MemoryManager* getMemoryManager() const { return fMemoryManager; }

    // -----------------------------------------------------------------------
    //  Match methods
    // -----------------------------------------------------------------------
    /**
      * Returns true if XPath has been matched.
      */
    int isMatched();
    virtual int getInitialDepth() const;

    // -----------------------------------------------------------------------
    //  XMLDocumentHandler methods
    // -----------------------------------------------------------------------
    virtual void startDocumentFragment();
    virtual void startElement(const XMLElementDecl& elemDecl,
                              const unsigned int urlId,
                              const XMLCh* const elemPrefix,
                              const RefVectorOf<XMLAttr>& attrList,
                              const unsigned int attrCount);
    virtual void endElement(const XMLElementDecl& elemDecl,
                            const XMLCh* const elemContent);

protected:

    enum
    {
        XP_MATCHED = 1        // matched any way
        , XP_MATCHED_A = 3    // matched on the attribute axis
        , XP_MATCHED_D = 5    // matched on the descendant-or-self axixs
        , XP_MATCHED_DP = 13  // matched some previous (ancestor) node on the
                              // descendant-or-self-axis, but not this node
    };

    // -----------------------------------------------------------------------
    //  Match methods
    // -----------------------------------------------------------------------
    /**
      * This method is called when the XPath handler matches the XPath
      * expression. Subclasses can override this method to provide default
      * handling upon a match.
      */
    virtual void matched(const XMLCh* const content,
                         DatatypeValidator* const dv, const bool isNil);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XPathMatcher(const XPathMatcher&);
    XPathMatcher& operator=(const XPathMatcher&);

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    void init(XercesXPath* const xpath);
    void cleanUp();

    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fMatched
    //      Indicates whether XPath has been matched or not
    //
    //  fNoMatchDepth
    //      Indicates whether matching is successful for the given xpath
    //      expression.
    //
    //  fCurrentStep
    //      Stores current step.
    //
    //  fStepIndexes
    //      Integer stack of step indexes.
    //
    //  fLocationPaths
    //  fLocationPathSize
    //      XPath location path, and its size.
    //
    //  fIdentityConstraint
    //      The identity constraint we're the matcher for.  Only used for
    //      selectors.
    //
    // -----------------------------------------------------------------------
    unsigned int                     fLocationPathSize;
    int*                             fMatched;
    int*                             fNoMatchDepth;
    int*                             fCurrentStep;
    RefVectorOf<ValueStackOf<int> >* fStepIndexes;
    RefVectorOf<XercesLocationPath>* fLocationPaths;
    IdentityConstraint*              fIdentityConstraint;
    MemoryManager*                   fMemoryManager;
};

// ---------------------------------------------------------------------------
//  XPathMatcher: Helper methods
// ---------------------------------------------------------------------------
inline void XPathMatcher::cleanUp() {

    fMemoryManager->deallocate(fMatched);//delete [] fMatched;
    fMemoryManager->deallocate(fNoMatchDepth);//delete [] fNoMatchDepth;
    fMemoryManager->deallocate(fCurrentStep);//delete [] fCurrentStep;
    delete fStepIndexes;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file XPathMatcher.hpp
  */

