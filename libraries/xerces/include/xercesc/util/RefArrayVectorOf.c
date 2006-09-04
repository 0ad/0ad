/*
 * Copyright 1999-2004 The Apache Software Foundation.
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
