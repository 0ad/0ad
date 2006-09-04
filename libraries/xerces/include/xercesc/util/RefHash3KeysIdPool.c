/*
 * Copyright 2001,2004 The Apache Software Foundation.
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
 * $Id: RefHash3KeysIdPool.c 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Include
// ---------------------------------------------------------------------------
#if defined(XERCES_TMPLSINC)
#include <xercesc/util/RefHash3KeysIdPool.hpp>
#endif

#include <xercesc/util/NullPointerException.hpp>
#include <assert.h>
#include <new>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  RefHash3KeysIdPool: Constructors and Destructor
// ---------------------------------------------------------------------------
template <class TVal>
RefHash3KeysIdPool<TVal>::RefHash3KeysIdPool( const unsigned int modulus
                                            , const bool         adoptElems
                                            , const unsigned int initSize
                                            , MemoryManager* const manager) :
    fMemoryManager(manager)
    , fAdoptedElems(adoptElems)
    , fBucketList(0)
    , fHashModulus(modulus)
    , fHash(0)
    , fIdPtrs(0)
    , fIdPtrsCount(initSize)
    , fIdCounter(0)
{
    initialize(modulus);

    // create default hasher
#if defined (XML_GCC_VERSION) && (XML_GCC_VERSION < 29600)
		 fHash = new HashXMLCh();
#else
    fHash = new (fMemoryManager) HashXMLCh();
#endif

    //
    //  Allocate the initial id pointers array. We don't have to zero them
    //  out since the fIdCounter value tells us which ones are valid. The
    //  zeroth element is never used (and represents an invalid pool id.)
    //
    if (!fIdPtrsCount)
        fIdPtrsCount = 256;
    fIdPtrs = (TVal**) fMemoryManager->allocate(fIdPtrsCount * sizeof(TVal*)); //new TVal*[fIdPtrsCount];
    fIdPtrs[0] = 0;
}

template <class TVal>
RefHash3KeysIdPool<TVal>::RefHash3KeysIdPool( const unsigned int modulus
                                            , const bool         adoptElems
                                            , HashBase*          hashBase
                                            , const unsigned int initSize
                                            , MemoryManager* const manager) :
	fMemoryManager(manager)
    , fAdoptedElems(adoptElems)
    , fBucketList(0)
    , fHashModulus(modulus)
    , fHash(0)
    , fIdPtrs(0)
    , fIdPtrsCount(initSize)
    , fIdCounter(0)
{
    initialize(modulus);
    // set hasher
    fHash = hashBase;

    //
    //  Allocate the initial id pointers array. We don't have to zero them
    //  out since the fIdCounter value tells us which ones are valid. The
    //  zeroth element is never used (and represents an invalid pool id.)
    //
    if (!fIdPtrsCount)
        fIdPtrsCount = 256;
    fIdPtrs = (TVal**) fMemoryManager->allocate(fIdPtrsCount * sizeof(TVal*)); //new TVal*[fIdPtrsCount];
    fIdPtrs[0] = 0;
}

template <class TVal>
RefHash3KeysIdPool<TVal>::RefHash3KeysIdPool( const unsigned int modulus
                                            , const unsigned int initSize
                                            , MemoryManager* const manager) :
	fMemoryManager(manager)
    , fAdoptedElems(true)
    , fBucketList(0)
    , fHashModulus(modulus)
    , fHash(0)
    , fIdPtrs(0)
    , fIdPtrsCount(initSize)
    , fIdCounter(0)
{
    initialize(modulus);

    // create default hasher
#if defined (XML_GCC_VERSION) && (XML_GCC_VERSION < 29600)
		 fHash = new HashXMLCh();
#else
    fHash = new (fMemoryManager) HashXMLCh();
#endif    

    //
    //  Allocate the initial id pointers array. We don't have to zero them
    //  out since the fIdCounter value tells us which ones are valid. The
    //  zeroth element is never used (and represents an invalid pool id.)
    //
    if (!fIdPtrsCount)
        fIdPtrsCount = 256;
    fIdPtrs = (TVal**) fMemoryManager->allocate(fIdPtrsCount * sizeof(TVal*)); //new TVal*[fIdPtrsCount];
    fIdPtrs[0] = 0;
}

template <class TVal> void RefHash3KeysIdPool<TVal>::initialize(const unsigned int modulus)
{
	if (modulus == 0)
        ThrowXMLwithMemMgr(IllegalArgumentException, XMLExcepts::HshTbl_ZeroModulus, fMemoryManager);

    // Allocate the bucket list and zero them
    fBucketList = (RefHash3KeysTableBucketElem<TVal>**) fMemoryManager->allocate
    (
        fHashModulus * sizeof(RefHash3KeysTableBucketElem<TVal>*)
    ); //new RefHash3KeysTableBucketElem<TVal>*[fHashModulus];
    for (unsigned int index = 0; index < fHashModulus; index++)
        fBucketList[index] = 0;
}

template <class TVal> RefHash3KeysIdPool<TVal>::~RefHash3KeysIdPool()
{
    removeAll();

    // Then delete the bucket list & hasher & id pointers list
    fMemoryManager->deallocate(fIdPtrs); //delete [] fIdPtrs;
    fMemoryManager->deallocate(fBucketList); //delete [] fBucketList;
    delete fHash;
}


// ---------------------------------------------------------------------------
//  RefHash3KeysIdPool: Element management
// ---------------------------------------------------------------------------
template <class TVal> bool RefHash3KeysIdPool<TVal>::isEmpty() const
{
    // Just check the bucket list for non-empty elements
    for (unsigned int buckInd = 0; buckInd < fHashModulus; buckInd++)
    {
        if (fBucketList[buckInd] != 0)
            return false;
    }
    return true;
}

template <class TVal> bool RefHash3KeysIdPool<TVal>::
containsKey(const void* const key1, const int key2, const int key3) const
{
    unsigned int hashVal;
    const RefHash3KeysTableBucketElem<TVal>* findIt = findBucketElem(key1, key2, key3, hashVal);
    return (findIt != 0);
}

template <class TVal> void RefHash3KeysIdPool<TVal>::removeAll()
{
    // Clean up the buckets first
    for (unsigned int buckInd = 0; buckInd < fHashModulus; buckInd++)
    {
        // Get the bucket list head for this entry
        RefHash3KeysTableBucketElem<TVal>* curElem = fBucketList[buckInd];
        RefHash3KeysTableBucketElem<TVal>* nextElem;
        while (curElem)
        {
            // Save the next element before we hose this one
            nextElem = curElem->fNext;

            // If we adopted the data, then delete it too
            //    (Note:  the userdata hash table instance has data type of void *.
            //    This will generate compiler warnings here on some platforms, but they
            //    can be ignored since fAdoptedElements is false.
            if (fAdoptedElems)
                delete curElem->fData;

            // Then delete the current element and move forward
            // delete curElem;
            // destructor is empty...
            // curElem->~RefHash3KeysTableBucketElem();
            fMemoryManager->deallocate(curElem);
            curElem = nextElem;
        }

        // Clean out this entry
        fBucketList[buckInd] = 0;
    }

    // Reset the id counter
    fIdCounter = 0;
}


// ---------------------------------------------------------------------------
//  RefHash3KeysIdPool: Getters
// ---------------------------------------------------------------------------
template <class TVal> TVal*
RefHash3KeysIdPool<TVal>::getByKey(const void* const key1, const int key2, const int key3)
{
    unsigned int hashVal;
    RefHash3KeysTableBucketElem<TVal>* findIt = findBucketElem(key1, key2, key3, hashVal);
    if (!findIt)
        return 0;
    return findIt->fData;
}

template <class TVal> const TVal*
RefHash3KeysIdPool<TVal>::getByKey(const void* const key1, const int key2, const int key3) const
{
    unsigned int hashVal;
    const RefHash3KeysTableBucketElem<TVal>* findIt = findBucketElem(key1, key2, key3, hashVal);
    if (!findIt)
        return 0;
    return findIt->fData;
}

template <class TVal> TVal*
RefHash3KeysIdPool<TVal>::getById(const unsigned int elemId)
{
    // If its either zero or beyond our current id, its an error
    if (!elemId || (elemId > fIdCounter))
        ThrowXMLwithMemMgr(IllegalArgumentException, XMLExcepts::Pool_InvalidId, fMemoryManager);

    return fIdPtrs[elemId];
}

template <class TVal> const TVal*
RefHash3KeysIdPool<TVal>::getById(const unsigned int elemId) const
{
    // If its either zero or beyond our current id, its an error
    if (!elemId || (elemId > fIdCounter))
        ThrowXMLwithMemMgr(IllegalArgumentException, XMLExcepts::Pool_InvalidId, fMemoryManager);

    return fIdPtrs[elemId];
}

template <class TVal>
MemoryManager* RefHash3KeysIdPool<TVal>::getMemoryManager() const
{
    return fMemoryManager;
}

template <class TVal>
unsigned int RefHash3KeysIdPool<TVal>::getHashModulus() const
{
    return fHashModulus;
}

// ---------------------------------------------------------------------------
//  RefHash3KeysIdPool: Putters
// ---------------------------------------------------------------------------
template <class TVal> unsigned int
RefHash3KeysIdPool<TVal>::put(void* key1, int key2, int key3, TVal* const valueToAdopt)
{
    // First see if the key exists already
    unsigned int hashVal;
    RefHash3KeysTableBucketElem<TVal>* newBucket = findBucketElem(key1, key2, key3, hashVal);

    //
    //  If so,then update its value. If not, then we need to add it to
    //  the right bucket
    //
    if (newBucket)
    {
        if (fAdoptedElems)
            delete newBucket->fData;
        newBucket->fData = valueToAdopt;
        newBucket->fKey1 = key1;
        newBucket->fKey2 = key2;
        newBucket->fKey3 = key3;
    }
     else
    {
    // Revisit: the gcc compiler 2.95.x is generating an
    // internal compiler error message. So we use the default
    // memory manager for now.
#if defined (XML_GCC_VERSION) && (XML_GCC_VERSION < 29600)
        newBucket = new RefHash3KeysTableBucketElem<TVal>(key1, key2, key3, valueToAdopt, fBucketList[hashVal]);
#else
        newBucket =
            new (fMemoryManager->allocate(sizeof(RefHash3KeysTableBucketElem<TVal>)))
            RefHash3KeysTableBucketElem<TVal>(key1, key2, key3, valueToAdopt, fBucketList[hashVal]);
#endif
        fBucketList[hashVal] = newBucket;
    }

    //
    //  Give this new one the next available id and add to the pointer list.
    //  Expand the list if that is now required.
    //
    if (fIdCounter + 1 == fIdPtrsCount)
    {
        // Create a new count 1.5 times larger and allocate a new array
        unsigned int newCount = (unsigned int)(fIdPtrsCount * 1.5);
        TVal** newArray = (TVal**) fMemoryManager->allocate
        (
            newCount * sizeof(TVal*)
        ); //new TVal*[newCount];

        // Copy over the old contents to the new array
        memcpy(newArray, fIdPtrs, fIdPtrsCount * sizeof(TVal*));

        // Ok, toss the old array and store the new data
        fMemoryManager->deallocate(fIdPtrs); //delete [] fIdPtrs;
        fIdPtrs = newArray;
        fIdPtrsCount = newCount;
    }
    const unsigned int retId = ++fIdCounter;
    fIdPtrs[retId] = valueToAdopt;

    // Set the id on the passed element
    valueToAdopt->setId(retId);

    // Return the id that we gave to this element
    return retId;
}

// ---------------------------------------------------------------------------
//  RefHash3KeysIdPool: Private methods
// ---------------------------------------------------------------------------
template <class TVal> RefHash3KeysTableBucketElem<TVal>* RefHash3KeysIdPool<TVal>::
findBucketElem(const void* const key1, const int key2, const int key3, unsigned int& hashVal)
{
    // Hash the key
    hashVal = fHash->getHashVal(key1, fHashModulus, fMemoryManager);
    assert(hashVal < fHashModulus);

    // Search that bucket for the key
    RefHash3KeysTableBucketElem<TVal>* curElem = fBucketList[hashVal];
    while (curElem)
    {
		if (key2==curElem->fKey2 && key3==curElem->fKey3 && fHash->equals(key1, curElem->fKey1))
            return curElem;

        curElem = curElem->fNext;
    }
    return 0;
}

template <class TVal> const RefHash3KeysTableBucketElem<TVal>* RefHash3KeysIdPool<TVal>::
findBucketElem(const void* const key1, const int key2, const int key3, unsigned int& hashVal) const
{
    // Hash the key
    hashVal = fHash->getHashVal(key1, fHashModulus);
    assert(hashVal < fHashModulus);

    // Search that bucket for the key
    const RefHash3KeysTableBucketElem<TVal>* curElem = fBucketList[hashVal];
    while (curElem)
    {
        if (fHash->equals(key1, curElem->fKey1) && (key2==curElem->fKey2) && (key3==curElem->fKey3))
            return curElem;

        curElem = curElem->fNext;
    }
    return 0;
}


// ---------------------------------------------------------------------------
//  RefHash3KeysIdPoolEnumerator: Constructors and Destructor
// ---------------------------------------------------------------------------
template <class TVal> RefHash3KeysIdPoolEnumerator<TVal>::
RefHash3KeysIdPoolEnumerator(RefHash3KeysIdPool<TVal>* const toEnum
                             , const bool adopt
                             , MemoryManager* const manager)
	: fAdoptedElems(adopt), fCurIndex(0), fToEnum(toEnum), fMemoryManager(manager)
{
    if (!toEnum)
        ThrowXMLwithMemMgr(NullPointerException, XMLExcepts::CPtr_PointerIsZero, fMemoryManager);

    Reset();
    resetKey();
}

template <class TVal> RefHash3KeysIdPoolEnumerator<TVal>::~RefHash3KeysIdPoolEnumerator()
{
    if (fAdoptedElems)
        delete fToEnum;
}

template <class TVal> RefHash3KeysIdPoolEnumerator<TVal>::
RefHash3KeysIdPoolEnumerator(const RefHash3KeysIdPoolEnumerator<TVal>& toCopy) :
    XMLEnumerator<TVal>(toCopy)
    , XMemory(toCopy)
    , fAdoptedElems(toCopy.fAdoptedElems)
    , fCurIndex(toCopy.fCurIndex)
    , fToEnum(toCopy.fToEnum)
    , fCurElem(toCopy.fCurElem)
    , fCurHash(toCopy.fCurHash)    
    , fMemoryManager(toCopy.fMemoryManager)
{
}

// ---------------------------------------------------------------------------
//  RefHash3KeysIdPoolEnumerator: Enum interface
// ---------------------------------------------------------------------------
template <class TVal> bool RefHash3KeysIdPoolEnumerator<TVal>::hasMoreElements() const
{
    // If our index is zero or past the end, then we are done
    if (!fCurIndex || (fCurIndex > fToEnum->fIdCounter))
        return false;
    return true;
}

template <class TVal> TVal& RefHash3KeysIdPoolEnumerator<TVal>::nextElement()
{
    // If our index is zero or past the end, then we are done
    if (!fCurIndex || (fCurIndex > fToEnum->fIdCounter))
        ThrowXMLwithMemMgr(NoSuchElementException, XMLExcepts::Enum_NoMoreElements, fMemoryManager);

    // Return the current element and bump the index
    return *fToEnum->fIdPtrs[fCurIndex++];
}

template <class TVal> void RefHash3KeysIdPoolEnumerator<TVal>::Reset()
{
    //
    //  Find the next available bucket element in the pool. We use the id
    //  array since its very easy to enumerator through by just maintaining
    //  an index. If the id counter is zero, then its empty and we leave the
    //  current index to zero.
    //
    fCurIndex = fToEnum->fIdCounter ? 1:0;

}

template <class TVal> int RefHash3KeysIdPoolEnumerator<TVal>::size() const
{
    return fToEnum->fIdCounter;
}

template <class TVal> void RefHash3KeysIdPoolEnumerator<TVal>::resetKey()
{
    fCurHash = (unsigned int)-1;
    fCurElem = 0;
    findNext();
}

template <class TVal> bool RefHash3KeysIdPoolEnumerator<TVal>::hasMoreKeys() const
{
    //
    //  If our current has is at the max and there are no more elements
    //  in the current bucket, then no more elements.
    //
    if (!fCurElem && (fCurHash == fToEnum->fHashModulus))
        return false;

    return true;
}

template <class TVal> void RefHash3KeysIdPoolEnumerator<TVal>::nextElementKey(void*& retKey1, int& retKey2, int& retKey3)
{
    // Make sure we have an element to return
    if (!hasMoreKeys())
        ThrowXMLwithMemMgr(NoSuchElementException, XMLExcepts::Enum_NoMoreElements, fMemoryManager);

    //
    //  Save the current element, then move up to the next one for the
    //  next time around.
    //
    RefHash3KeysTableBucketElem<TVal>* saveElem = fCurElem;
    findNext();

    retKey1 = saveElem->fKey1;
    retKey2 = saveElem->fKey2;
    retKey3 = saveElem->fKey3;

    return;
}

template <class TVal> void RefHash3KeysIdPoolEnumerator<TVal>::findNext()
{
    //
    //  If there is a current element, move to its next element. If this
    //  hits the end of the bucket, the next block will handle the rest.
    //
    if (fCurElem)
        fCurElem = fCurElem->fNext;

    //
    //  If the current element is null, then we have to move up to the
    //  next hash value. If that is the hash modulus, then we cannot
    //  go further.
    //
    if (!fCurElem)
    {
        fCurHash++;
        if (fCurHash == fToEnum->fHashModulus)
            return;

        // Else find the next non-empty bucket
        while (fToEnum->fBucketList[fCurHash]==0)
        {
            // Bump to the next hash value. If we max out return
            fCurHash++;
            if (fCurHash == fToEnum->fHashModulus)
                return;
        }
        fCurElem = fToEnum->fBucketList[fCurHash];
    }
}

XERCES_CPP_NAMESPACE_END
