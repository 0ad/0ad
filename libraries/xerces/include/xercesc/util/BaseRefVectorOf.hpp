/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2002 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */
#if !defined(ABSTRACTVECTOROF_HPP)
#define ABSTRACTVECTOROF_HPP

#include <xercesc/util/ArrayIndexOutOfBoundsException.hpp>
#include <xercesc/util/XMLEnumerator.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemoryManager.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/** 
 * Abstract base class for the xerces internal representation of Vector. 
 * 
 * The destructor is abstract, forcing each of RefVectorOf and
 * RefArrayVectorOf to implement their own appropriate one.
 *
 */
template <class TElem> class BaseRefVectorOf : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    BaseRefVectorOf
    (
          const unsigned int maxElems
        , const bool adoptElems = true
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    virtual ~BaseRefVectorOf() = 0;


    // -----------------------------------------------------------------------
    //  Element management
    // -----------------------------------------------------------------------
    void addElement(TElem* const toAdd);
    virtual void setElementAt(TElem* const toSet, const unsigned int setAt);
    void insertElementAt(TElem* const toInsert, const unsigned int insertAt);
    TElem* orphanElementAt(const unsigned int orphanAt);
    virtual void removeAllElements();
    virtual void removeElementAt(const unsigned int removeAt);
    virtual void removeLastElement();
    bool containsElement(const TElem* const toCheck);
    virtual void cleanup();
    void reinitialize();


    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    unsigned int curCapacity() const;
    const TElem* elementAt(const unsigned int getAt) const;
    TElem* elementAt(const unsigned int getAt);
    unsigned int size() const;
    MemoryManager* getMemoryManager() const;


    // -----------------------------------------------------------------------
    //  Miscellaneous
    // -----------------------------------------------------------------------
    void ensureExtraCapacity(const unsigned int length);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    BaseRefVectorOf(const BaseRefVectorOf<TElem>& copy);
    BaseRefVectorOf& operator=(const BaseRefVectorOf<TElem>& copy);       

protected:
    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    bool            fAdoptedElems;
    unsigned int    fCurCount;
    unsigned int    fMaxCount;
    TElem**         fElemList;
    MemoryManager*  fMemoryManager;
};


//
//  An enumerator for a vector. It derives from the basic enumerator
//  class, so that value vectors can be generically enumerated.
//
template <class TElem> class BaseRefVectorEnumerator : public XMLEnumerator<TElem>, public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    BaseRefVectorEnumerator
    (
        BaseRefVectorOf<TElem>* const   toEnum
        , const bool adopt = false
    );
    virtual ~BaseRefVectorEnumerator();

    BaseRefVectorEnumerator(const BaseRefVectorEnumerator<TElem>& copy);
    // -----------------------------------------------------------------------
    //  Enum interface
    // -----------------------------------------------------------------------
    bool hasMoreElements() const;
    TElem& nextElement();
    void Reset();


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------        
    BaseRefVectorEnumerator& operator=(const BaseRefVectorEnumerator<TElem>& copy);    
    // -----------------------------------------------------------------------
    //  Data Members
    //
    //  fAdopted
    //      Indicates whether we have adopted the passed vector. If so then
    //      we delete the vector when we are destroyed.
    //
    //  fCurIndex
    //      This is the current index into the vector.
    //
    //  fToEnum
    //      The reference vector being enumerated.
    // -----------------------------------------------------------------------
    bool                fAdopted;
    unsigned int        fCurIndex;
    BaseRefVectorOf<TElem>*    fToEnum;
};

XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/util/BaseRefVectorOf.c>
#endif

#endif
