/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2003 The Apache Software Foundation.  All rights
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

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#if defined(XERCES_TMPLSINC)
#include "RefArrayVectorOf.hpp"
#endif

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  RefArrayVectorOf: Constructor and Destructor
// ---------------------------------------------------------------------------
template <class TElem>
RefArrayVectorOf<TElem>::RefArrayVectorOf( const unsigned int maxElems
                                         , const bool adoptElems
                                         , MemoryManager* const manager)
    : BaseRefVectorOf<TElem>(maxElems, adoptElems, manager)
{
}


template <class TElem> RefArrayVectorOf<TElem>::~RefArrayVectorOf()
{
    if (this->fAdoptedElems)
    {
      for (unsigned int index = 0; index < this->fCurCount; index++)
            this->fMemoryManager->deallocate(this->fElemList[index]);//delete[] fElemList[index];
    }
    this->fMemoryManager->deallocate(this->fElemList);//delete [] fElemList;
}

template <class TElem> void
RefArrayVectorOf<TElem>::setElementAt(TElem* const toSet, const unsigned int setAt)
{
    if (setAt >= this->fCurCount)
        ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::Vector_BadIndex, this->fMemoryManager);

    if (this->fAdoptedElems)
        this->fMemoryManager->deallocate(this->fElemList[setAt]);

    this->fElemList[setAt] = toSet;
}

template <class TElem> void RefArrayVectorOf<TElem>::removeAllElements()
{
    for (unsigned int index = 0; index < this->fCurCount; index++)
    {
        if (this->fAdoptedElems)
          this->fMemoryManager->deallocate(this->fElemList[index]);

        // Keep unused elements zero for sanity's sake
        this->fElemList[index] = 0;
    }
    this->fCurCount = 0;
}

template <class TElem> void RefArrayVectorOf<TElem>::
removeElementAt(const unsigned int removeAt)
{
    if (removeAt >= this->fCurCount)
        ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::Vector_BadIndex, this->fMemoryManager);

    if (this->fAdoptedElems)
        this->fMemoryManager->deallocate(this->fElemList[removeAt]);

    // Optimize if its the last element
    if (removeAt == this->fCurCount-1)
    {
        this->fElemList[removeAt] = 0;
        this->fCurCount--;
        return;
    }

    // Copy down every element above remove point
    for (unsigned int index = removeAt; index < this->fCurCount-1; index++)
        this->fElemList[index] = this->fElemList[index+1];

    // Keep unused elements zero for sanity's sake
    this->fElemList[this->fCurCount-1] = 0;

    // And bump down count
    this->fCurCount--;
}

template <class TElem> void RefArrayVectorOf<TElem>::removeLastElement()
{
    if (!this->fCurCount)
        return;
    this->fCurCount--;

    if (this->fAdoptedElems)
        this->fMemoryManager->deallocate(this->fElemList[this->fCurCount]);
}

template <class TElem> void RefArrayVectorOf<TElem>::cleanup()
{
    if (this->fAdoptedElems)
    {
        for (unsigned int index = 0; index < this->fCurCount; index++)
            this->fMemoryManager->deallocate(this->fElemList[index]);
    }
    this->fMemoryManager->deallocate(this->fElemList);//delete [] fElemList;
}

XERCES_CPP_NAMESPACE_END
