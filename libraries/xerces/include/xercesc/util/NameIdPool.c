/*
 * Copyright 1999-2000,2004 The Apache Software Foundation.
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
 * $Id: NameIdPool.c 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#if defined(XERCES_TMPLSINC)
#include <xercesc/util/NameIdPool.hpp>
#endif

#include <xercesc/util/IllegalArgumentException.hpp>
#include <xercesc/util/NoSuchElementException.hpp>
#include <xercesc/util/RuntimeException.hpp>
#include <new>
#include <assert.h>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  NameIdPoolBucketElem: Constructors and Destructor
// ---------------------------------------------------------------------------
template <class TElem> NameIdPoolBucketElem<TElem>::
NameIdPoolBucketElem(TElem* const                           value
                    , NameIdPoolBucketElem<TElem>* const    next) :
    fData(value)
    , fNext(next)
{
}

template <class TElem> NameIdPoolBucketElem<TElem>::~NameIdPoolBucketElem()
{
    // Nothing to do
}


// ---------------------------------------------------------------------------
//  NameIdPool: Constructors and Destructor
// ---------------------------------------------------------------------------
template <class TElem>
NameIdPool<TElem>::NameIdPool( const unsigned int hashModulus
                             , const unsigned int initSize
                             , MemoryManager* const manager) :
    fMemoryManager(manager)
    , fBucketList(0)
    , fIdPtrs(0)
    , fIdPtrsCount(initSize)
    , fIdCounter(0)
    , fHashModulus(hashModulus)
{
    if (!fHashModulus)
        ThrowXMLwithMemMgr(IllegalArgumentException, XMLExcepts::Pool_ZeroModulus, fMemoryManager);

    // Allocate the bucket list and zero them
    fBucketList = (NameIdPoolBucketElem<TElem>**) fMemoryManager->allocate
    (
        fHashModulus * sizeof(NameIdPoolBucketElem<TElem>*)
    ); //new NameIdPoolBucketElem<TElem>*[fHashModulus];
    for (unsigned int index = 0; index < fHashModulus; index++)
        fBucketList[index] = 0;

    //
    //  Allocate the initial id pointers array. We don't have to zero them
    //  out since the fIdCounter value tells us which ones are valid. The
    //  zeroth element is never used (and represents an invalid pool id.)
    //
    if (!fIdPtrsCount)
        fIdPtrsCount = 256;
    fIdPtrs = (TElem**) fMemoryManager->allocate
    (
        fIdPtrsCount * sizeof(TElem*)
    ); //new TElem*[fIdPtrsCount];
    fIdPtrs[0] = 0;
}

template <class TElem> NameIdPool<TElem>::~NameIdPool()
{
    //
    //  Delete the id pointers list. The stuff it points to will be cleaned
    //  up when we clean the bucket lists.
    //
    fMemoryManager->deallocate(fIdPtrs); //delete [] fIdPtrs;

    // Remove all elements then delete the bucket list
    removeAll();
    fMemoryManager->deallocate(fBucketList); //delete [] fBucketList;
}


// ---------------------------------------------------------------------------
//  NameIdPool: Element management
// ---------------------------------------------------------------------------
template <class TElem> bool
NameIdPool<TElem>::containsKey(const XMLCh* const key) const
{
    unsigned int hashVal;
    const NameIdPoolBucketElem<TElem>* findIt = findBucketElem(key, hashVal);
    return (findIt != 0);
}


template <class TElem> void NameIdPool<TElem>::removeAll()
{
    // Clean up the buckets first
    for (unsigned int buckInd = 0; buckInd < fHashModulus; buckInd++)
    {
        NameIdPoolBucketElem<TElem>* curElem = fBucketList[buckInd];
        NameIdPoolBucketElem<TElem>* nextElem;
        while (curElem)
        {
            // Save the next element before we hose this one
            nextElem = curElem->fNext;

            delete curElem->fData;
            // destructor is empty...
            // curElem->~NameIdPoolBucketElem();
            fMemoryManager->deallocate(curElem);

            curElem = nextElem;
        }

        // Empty out the bucket
        fBucketList[buckInd] = 0;
    }

    // Reset the id counter
    fIdCounter = 0;
}


// ---------------------------------------------------------------------------
//  NameIdPool: Getters
// ---------------------------------------------------------------------------
template <class TElem> TElem*
NameIdPool<TElem>::getByKey(const XMLCh* const key)
{
    unsigned int hashVal;
    NameIdPoolBucketElem<TElem>* findIt = findBucketElem(key, hashVal);
    if (!findIt)
        return 0;
    return findIt->fData;
}

template <class TElem> const TElem*
NameIdPool<TElem>::getByKey(const XMLCh* const key) const
{
    unsigned int hashVal;
    const NameIdPoolBucketElem<TElem>* findIt = findBucketElem(key, hashVal);
    if (!findIt)
        return 0;
    return findIt->fData;
}

template <class TElem> TElem*
NameIdPool<TElem>::getById(const unsigned int elemId)
{
    // If its either zero or beyond our current id, its an error
    if (!elemId || (elemId > fIdCounter))
        ThrowXMLwithMemMgr(IllegalArgumentException, XMLExcepts::Pool_InvalidId, fMemoryManager);

    return fIdPtrs[elemId];
}

template <class TElem>
const TElem* NameIdPool<TElem>::getById(const unsigned int elemId) const
{
    // If its either zero or beyond our current id, its an error
    if (!elemId || (elemId > fIdCounter))
        ThrowXMLwithMemMgr(IllegalArgumentException, XMLExcepts::Pool_InvalidId, fMemoryManager);

    return fIdPtrs[elemId];
}

template <class TElem>
MemoryManager* NameIdPool<TElem>::getMemoryManager() const
{
    return fMemoryManager;
}

// ---------------------------------------------------------------------------
//  NameIdPool: Setters
// ---------------------------------------------------------------------------
template <class TElem>
unsigned int NameIdPool<TElem>::put(TElem* const elemToAdopt)
{
    // First see if the key exists already. If so, its an error
    unsigned int hashVal;
    if (findBucketElem(elemToAdopt->getKey(), hashVal))
    {
        ThrowXMLwithMemMgr1
        (
            IllegalArgumentException
            , XMLExcepts::Pool_ElemAlreadyExists
            , elemToAdopt->getKey()
            , fMemoryManager
        );
    }

    // Create a new bucket element and add it to the appropriate list
    NameIdPoolBucketElem<TElem>* newBucket =
        new (fMemoryManager->allocate(sizeof(NameIdPoolBucketElem<TElem>)))
        NameIdPoolBucketElem<TElem>(elemToAdopt,fBucketList[hashVal]);
    fBucketList[hashVal] = newBucket;

    //
    //  Give this new one the next available id and add to the pointer list.
    //  Expand the list if that is now required.
    //
    if (fIdCounter + 1 == fIdPtrsCount)
    {
        // Create a new count 1.5 times larger and allocate a new array
        unsigned int newCount = (unsigned int)(fIdPtrsCount * 1.5);
        TElem** newArray = (TElem**) fMemoryManager->allocate
        (
            newCount * sizeof(TElem*)
        ); //new TElem*[newCount];

        // Copy over the old contents to the new array
        memcpy(newArray, fIdPtrs, fIdPtrsCount * sizeof(TElem*));

        // Ok, toss the old array and store the new data
        fMemoryManager->deallocate(fIdPtrs); //delete [] fIdPtrs;
        fIdPtrs = newArray;
        fIdPtrsCount = newCount;
    }
    const unsigned int retId = ++fIdCounter;
    fIdPtrs[retId] = elemToAdopt;

    // Set the id on the passed element
    elemToAdopt->setId(retId);

    // Return the id that we gave to this element
    return retId;
}


// ---------------------------------------------------------------------------
//  NameIdPool: Private methods
// ---------------------------------------------------------------------------
template <class TElem>
NameIdPoolBucketElem<TElem>* NameIdPool<TElem>::
findBucketElem(const XMLCh* const key, unsigned int& hashVal)
{
    // Hash the key
    hashVal = XMLString::hash(key, fHashModulus, fMemoryManager);

    assert(hashVal < fHashModulus);        

    // Search that bucket for the key
    NameIdPoolBucketElem<TElem>* curElem = fBucketList[hashVal];
    while (curElem)
    {
        if (XMLString::equals(key, curElem->fData->getKey()))
            return curElem;
        curElem = curElem->fNext;
    }
    return 0;
}

template <class TElem>
const NameIdPoolBucketElem<TElem>* NameIdPool<TElem>::
findBucketElem(const XMLCh* const key, unsigned int& hashVal) const
{
    // Hash the key
    hashVal = XMLString::hash(key, fHashModulus, fMemoryManager);

    assert(hashVal < fHashModulus);

    // Search that bucket for the key
    const NameIdPoolBucketElem<TElem>* curElem = fBucketList[hashVal];
    while (curElem)
    {
        if (XMLString::equals(key, curElem->fData->getKey()))
            return curElem;

        curElem = curElem->fNext;
    }
    return 0;
}



// ---------------------------------------------------------------------------
//  NameIdPoolEnumerator: Constructors and Destructor
// ---------------------------------------------------------------------------
template <class TElem> NameIdPoolEnumerator<TElem>::
NameIdPoolEnumerator(NameIdPool<TElem>* const toEnum
                     , MemoryManager* const manager) :

    XMLEnumerator<TElem>()
    , fCurIndex(0)
    , fToEnum(toEnum)
    , fMemoryManager(manager)
{
        Reset();
}

template <class TElem> NameIdPoolEnumerator<TElem>::
NameIdPoolEnumerator(const NameIdPoolEnumerator<TElem>& toCopy) :
    XMLEnumerator<TElem>(toCopy)
    , XMemory(toCopy)
    , fCurIndex(toCopy.fCurIndex)
    , fToEnum(toCopy.fToEnum)
    , fMemoryManager(toCopy.fMemoryManager)
{
}

template <class TElem> NameIdPoolEnumerator<TElem>::~NameIdPoolEnumerator()
{
    // We don't own the pool being enumerated, so no cleanup required
}


// ---------------------------------------------------------------------------
//  NameIdPoolEnumerator: Public operators
// ---------------------------------------------------------------------------
template <class TElem> NameIdPoolEnumerator<TElem>& NameIdPoolEnumerator<TElem>::
operator=(const NameIdPoolEnumerator<TElem>& toAssign)
{
    if (this == &toAssign)
        return *this;
    fMemoryManager = toAssign.fMemoryManager;
    fCurIndex      = toAssign.fCurIndex;
    fToEnum        = toAssign.fToEnum;
    return *this;
}

// ---------------------------------------------------------------------------
//  NameIdPoolEnumerator: Enum interface
// ---------------------------------------------------------------------------
template <class TElem> bool NameIdPoolEnumerator<TElem>::
hasMoreElements() const
{
    // If our index is zero or past the end, then we are done
    if (!fCurIndex || (fCurIndex > fToEnum->fIdCounter))
        return false;
    return true;
}

template <class TElem> TElem& NameIdPoolEnumerator<TElem>::nextElement()
{
    // If our index is zero or past the end, then we are done
    if (!fCurIndex || (fCurIndex > fToEnum->fIdCounter))
        ThrowXMLwithMemMgr(NoSuchElementException, XMLExcepts::Enum_NoMoreElements, fMemoryManager);

    // Return the current element and bump the index
    return *fToEnum->fIdPtrs[fCurIndex++];
}


template <class TElem> void NameIdPoolEnumerator<TElem>::Reset()
{
    //
    //  Find the next available bucket element in the pool. We use the id
    //  array since its very easy to enumerator through by just maintaining
    //  an index. If the id counter is zero, then its empty and we leave the
    //  current index to zero.
    //
    fCurIndex = fToEnum->fIdCounter ? 1:0;
}

template <class TElem> int NameIdPoolEnumerator<TElem>::size() const
{
    return fToEnum->fIdCounter;
}

XERCES_CPP_NAMESPACE_END
