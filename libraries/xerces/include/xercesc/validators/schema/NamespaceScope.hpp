/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001 The Apache Software Foundation.  All rights
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
 * $Id: NamespaceScope.hpp,v 1.5 2003/05/16 21:43:21 knoaman Exp $
 */

#if !defined(NAMESPACESCOPE_HPP)
#define NAMESPACESCOPE_HPP

#include <xercesc/util/StringPool.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
// NamespaceScope provides a data structure for mapping namespace prefixes
// to their URI's. The mapping accurately reflects the scoping of namespaces
// at a particular instant in time.
//

class VALIDATORS_EXPORT NamespaceScope : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Class specific data types
    //
    //  These really should be private, but some of the compilers we have to
    //  support are too dumb to deal with that.
    //
    //  PrefMapElem
    //      fURIId is the id of the URI from the validator's URI map. The
    //      fPrefId is the id of the prefix from our own prefix pool. The
    //      namespace stack consists of these elements.
    //
    //  StackElem
    //      The fMapCapacity is how large fMap has grown so far. fMapCount
    //      is how many of them are valid right now.
    // -----------------------------------------------------------------------
    struct PrefMapElem : public XMemory
    {
        unsigned int        fPrefId;
        unsigned int        fURIId;
    };

    struct StackElem : public XMemory
    {
        PrefMapElem*        fMap;
        unsigned int        fMapCapacity;
        unsigned int        fMapCount;
    };


    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    NamespaceScope(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~NamespaceScope();


    // -----------------------------------------------------------------------
    //  Stack access
    // -----------------------------------------------------------------------
    unsigned int increaseDepth();
    unsigned int decreaseDepth();

    // -----------------------------------------------------------------------
    //  Prefix map methods
    // -----------------------------------------------------------------------
    void addPrefix(const XMLCh* const prefixToAdd,
                   const unsigned int uriId);

    unsigned int getNamespaceForPrefix(const XMLCh* const prefixToMap) const;
    unsigned int getNamespaceForPrefix(const XMLCh* const prefixToMap,
                                       const int depthLevel) const;


    // -----------------------------------------------------------------------
    //  Miscellaneous methods
    // -----------------------------------------------------------------------
    bool isEmpty() const;
    void reset(const unsigned int emptyId);


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    NamespaceScope(const NamespaceScope&);
    NamespaceScope& operator=(const NamespaceScope&);


    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------
    void expandMap(StackElem* const toExpand);
    void expandStack();


    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fEmptyNamespaceId
    //      This is the special URI id for the "" namespace, which is magic
    //      because of the xmlns="" operation.
    //
    //  fPrefixPool
    //      This is the prefix pool where prefixes are hashed and given unique
    //      ids. These ids are used to track prefixes in the element stack.
    //
    //  fStack
    //  fStackCapacity
    //  fStackTop
    //      This the stack array. Its an array of pointers to StackElem
    //      structures. The capacity is the current high water mark of the
    //      stack. The top is the current top of stack (i.e. the part of it
    //      being used.)
    // -----------------------------------------------------------------------
    unsigned int  fEmptyNamespaceId;
    unsigned int  fStackCapacity;
    unsigned int  fStackTop;
    XMLStringPool fPrefixPool;
    StackElem**   fStack;
    MemoryManager* fMemoryManager;
};


// ---------------------------------------------------------------------------
//  NamespaceScope: Stack access
// ---------------------------------------------------------------------------
inline unsigned int
NamespaceScope::getNamespaceForPrefix(const XMLCh* const prefixToMap) const {

    return getNamespaceForPrefix(prefixToMap, (int)(fStackTop - 1));
}

// ---------------------------------------------------------------------------
//  NamespaceScope: Miscellaneous methods
// ---------------------------------------------------------------------------
inline bool NamespaceScope::isEmpty() const
{
    return (fStackTop == 0);
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file NameSpaceScope.hpp
  */

