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
 * $Id: XPathMatcher.hpp,v 1.7 2004/01/29 11:52:32 cargilld Exp $
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

