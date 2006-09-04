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
 * $Id: RefHash2KeysTableOf.c 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Include
// ---------------------------------------------------------------------------
#if defined(XERCES_TMPLSINC)
#include <xercesc/util/RefHash2KeysTableOf.hpp>
#endif

#include <xercesc/util/Janitor.hpp>
#include <xercesc/util/NullPointerException.hpp>
#include <assert.h>
#include <new>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  RefHash2KeysTableOf: Constructors and Destructor
// ---------------------------------------------------------------------------
template <class TVal>
RefHash2KeysTableOf<TVal>::RefHash2KeysTableOf( const unsigned int   modulus
                                              , const bool           adoptElems
                                              , MemoryManager* const manager)
    : fMemoryManager(manager)
	, fAdoptedElems(adoptElems)
    , fBucketList(0)
    , fHashModulus(modulus)
    , fCount(0)
    , fHash(0)
{
    initialize(modulus);
	
	// create default hasher
	fHash = new (fMemoryManager) HashXMLCh();
}

template <class TVal>
RefHash2KeysTableOf<TVal>::RefHash2KeysTableOf( const unsigned int   modulus
                                              , const bool           adoptElems
                                              , HashBase*            hashBase
                                              , MemoryManager* const manager)
	: fMemoryManager(manager)
    , fAdoptedElems(adoptElems)
    , fBucketList(0)
    , fHashModulus(modulus)
    , fCount(0)
    , fHash(0)
{
	initialize(modulus);
	// set hasher
	fHash = hashBase;
}

template <class TVal>
RefHash2KeysTableOf<TVal>::RefHash2KeysTableOf(const unsigned int modulus,
                                               MemoryManager* const manager)
	: fMemoryManager(manager)
    , fAdoptedElems(true)
    , fBucketList(0)
    , fHashModulus(modulus)
    , fCount(0)
    , fHash(0)
{
	initialize(modulus);

	// create default hasher
	fHash = new (fMemoryManager) HashXMLCh();
}

template <class TVal>
void RefHash2KeysTableOf<TVal>::initialize(const unsigned int modulus)
{
	if (modulus == 0)
        ThrowXMLwithMemMgr(IllegalArgumentException, XMLExcepts::HshTbl_ZeroModulus, fMemoryManager);

    // Allocate the bucket list and zero them
    fBucketList = (RefHash2KeysTableBucketElem<TVal>**) fMemoryManager->allocate
    (
        fHashModulus * sizeof(RefHash2KeysTableBucketElem<TVal>*)
    ); //new RefHash2KeysTableBucketElem<TVal>*[fHashModulus];
    for (unsigned int index = 0; index < fHashModulus; index++)
        fBucketList[index] = 0;
}

template <class TVal> RefHash2KeysTableOf<TVal>::~RefHash2KeysTableOf()
{
    removeAll();

    // Then delete the bucket list & hasher
    fMemoryManager->deallocate(fBucketList); //delete [] fBucketList;
	delete fHash;
}


// ---------------------------------------------------------------------------
//  RefHash2KeysTableOf: Element management
// ---------------------------------------------------------------------------
template <class TVal> bool RefHash2KeysTableOf<TVal>::isEmpty() const
{
    return (fCount==0);
}

template <class TVal> bool RefHash2KeysTableOf<TVal>::
containsKey(const void* const key1, const int key2) const
{
    unsigned int hashVal;
    const RefHash2KeysTableBucketElem<TVal>* findIt = findBucketElem(key1, key2, hashVal);
    return (findIt != 0);
}

template <class TVal> void RefHash2KeysTableOf<TVal>::
removeKey(const void* const key1, const int key2)
{
    // Hash the key
    unsigned int hashVal = fHash->getHashVal(key1, fHashModulus);
    assert(hashVal < fHashModulus);

    //
    //  Search the given bucket for this key. Keep up with the previous
    //  element so we can patch around it.
    //
    RefHash2KeysTableBucketElem<TVal>* curElem = fBucketList[hashVal];
    RefHash2KeysTableBucketElem<TVal>* lastElem = 0;

    while (curElem)
    {
        if (fHash->equals(key1, curElem->fKey1) && (key2==curElem->fKey2))
        {
            if (!lastElem)
            {
                // It was the first in the bucket
                fBucketList[hashVal] = curElem->fNext;
            }
            else
            {
                // Patch around the current element
                lastElem->fNext = curElem->fNext;
            }

            // If we adopted the elements, then delete the data
            if (fAdoptedElems)
                delete curElem->fData;

            // Delete the current element
            // delete curElem;
            // destructor is empty...
            // curElem->~RefHash2KeysTableBucketElem();
            fMemoryManager->deallocate(curElem);            
            fCount--;
            return;
        }

        // Move both pointers upwards
        lastElem = curElem;
        curElem = curElem->fNext;
    }

    // We never found that key
    ThrowXMLwithMemMgr(NoSuchElementException, XMLExcepts::HshTbl_NoSuchKeyExists, fMemoryManager);
}

template <class TVal> void RefHash2KeysTableOf<TVal>::
removeKey(const void* const key1)
{
    // Hash the key
    unsigned int hashVal = fHash->getHashVal(key1, fHashModulus);
    assert(hashVal < fHashModulus);

    //
    //  Search the given bucket for this key. Keep up with the previous
    //  element so we can patch around it.
    //
    RefHash2KeysTableBucketElem<TVal>* curElem = fBucketList[hashVal];
    RefHash2KeysTableBucketElem<TVal>* lastElem = 0;

    while (curElem)
    {
        if (fHash->equals(key1, curElem->fKey1))
        {
            if (!lastElem)
            {
                // It was the first in the bucket
                fBucketList[hashVal] = curElem->fNext;
            }
            else
            {
                // Patch around the current element
                lastElem->fNext = curElem->fNext;
            }

            // If we adopted the elements, then delete the data
            if (fAdoptedElems)
                delete curElem->fData;

            RefHash2KeysTableBucketElem<TVal>* toBeDeleted=curElem;
            curElem = curElem->fNext;

            // Delete the current element
            // delete curElem;
            // destructor is empty...
            // curElem->~RefHash2KeysTableBucketElem();
            fMemoryManager->deallocate(toBeDeleted);
            fCount--;
        }
        else
        {
            // Move both pointers upwards
            lastElem = curElem;
            curElem = curElem->fNext;
        }
    }
}

template <class TVal> void RefHash2KeysTableOf<TVal>::removeAll()
{
    if(isEmpty())
        return;

    // Clean up the buckets first
    for (unsigned int buckInd = 0; buckInd < fHashModulus; buckInd++)
    {
        // Get the bucket list head for this entry
        RefHash2KeysTableBucketElem<TVal>* curElem = fBucketList[buckInd];
        RefHash2KeysTableBucketElem<TVal>* nextElem;
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
            // destructor is empty...
            // curElem->~RefHash2KeysTableBucketElem();
            fMemoryManager->deallocate(curElem);
            curElem = nextElem;
        }

        // Clean out this entry
        fBucketList[buckInd] = 0;
    }
    fCount=0;
}

// this function transfer the data from key1 to key2
template <class TVal> void RefHash2KeysTableOf<TVal>::transferElement(const void* const key1, void* key2)
{
    // Hash the key
    unsigned int hashVal = fHash->getHashVal(key1, fHashModulus);
    assert(hashVal < fHashModulus);

    //
    //  Search the given bucket for this key. Keep up with the previous
    //  element so we can patch around it.
    //
    RefHash2KeysTableBucketElem<TVal>* curElem = fBucketList[hashVal];
    RefHash2KeysTableBucketElem<TVal>* lastElem = 0;

    while (curElem)
    {
        // if this element has the same primary key, remove it and add it using the new primary key
        if (fHash->equals(key1, curElem->fKey1))
        {
            if (!lastElem)
            {
                // It was the first in the bucket
                fBucketList[hashVal] = curElem->fNext;
            }
            else
            {
                // Patch around the current element
                lastElem->fNext = curElem->fNext;
            }

            // this code comes from put(), but it doesn't update fCount
            unsigned int hashVal2;
            RefHash2KeysTableBucketElem<TVal>* newBucket = findBucketElem(key2, curElem->fKey2, hashVal2);
            if (newBucket)
            {
                if (fAdoptedElems)
                    delete newBucket->fData;
                newBucket->fData = curElem->fData;
		        newBucket->fKey1 = key2;
		        newBucket->fKey2 = curElem->fKey2;
            }
             else
            {
                newBucket =
                    new (fMemoryManager->allocate(sizeof(RefHash2KeysTableBucketElem<TVal>)))
                    RefHash2KeysTableBucketElem<TVal>(key2, curElem->fKey2, curElem->fData, fBucketList[hashVal2]);
                fBucketList[hashVal2] = newBucket;
            }

            RefHash2KeysTableBucketElem<TVal>* elemToDelete = curElem;
            
            // Update just curElem; lastElem must stay the same
            curElem = curElem->fNext;

            // Delete the current element
            // delete elemToDelete;
            // destructor is empty...
            // curElem->~RefHash2KeysTableBucketElem();
            fMemoryManager->deallocate(elemToDelete);
        }
        else
        {
            // Move both pointers upwards
            lastElem = curElem;
            curElem = curElem->fNext;
        }
    }
}



// ---------------------------------------------------------------------------
//  RefHash2KeysTableOf: Getters
// ---------------------------------------------------------------------------
template <class TVal> TVal* RefHash2KeysTableOf<TVal>::get(const void* const key1, const int key2)
{
    unsigned int hashVal;
    RefHash2KeysTableBucketElem<TVal>* findIt = findBucketElem(key1, key2, hashVal);
    if (!findIt)
        return 0;
    return findIt->fData;
}

template <class TVal> const TVal* RefHash2KeysTableOf<TVal>::
get(const void* const key1, const int key2) const
{
    unsigned int hashVal;
    const RefHash2KeysTableBucketElem<TVal>* findIt = findBucketElem(key1, key2, hashVal);
    if (!findIt)
        return 0;
    return findIt->fData;
}

template <class TVal>
MemoryManager* RefHash2KeysTableOf<TVal>::getMemoryManager() const
{
    return fMemoryManager;
}

template <class TVal>
unsigned int RefHash2KeysTableOf<TVal>::getHashModulus() const
{
    return fHashModulus;
}

// ---------------------------------------------------------------------------
//  RefHash2KeysTableOf: Putters
// ---------------------------------------------------------------------------
template <class TVal> void RefHash2KeysTableOf<TVal>::put(void* key1, int key2, TVal* const valueToAdopt)
{
    // Apply 4 load factor to find threshold.
    unsigned int threshold = fHashModulus * 4;
    
    // If we've grown too big, expand the table and rehash.
    if (fCount >= threshold)
        rehash();

    // First see if the key exists already
    unsigned int hashVal;
    RefHash2KeysTableBucketElem<TVal>* newBucket = findBucketElem(key1, key2, hashVal);

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
    }
     else
    {
        newBucket =
            new (fMemoryManager->allocate(sizeof(RefHash2KeysTableBucketElem<TVal>)))
            RefHash2KeysTableBucketElem<TVal>(key1, key2, valueToAdopt, fBucketList[hashVal]);
        fBucketList[hashVal] = newBucket;
        fCount++;
    }
}



// ---------------------------------------------------------------------------
//  RefHash2KeysTableOf: Private methods
// ---------------------------------------------------------------------------
template <class TVal> RefHash2KeysTableBucketElem<TVal>* RefHash2KeysTableOf<TVal>::
findBucketElem(const void* const key1, const int key2, unsigned int& hashVal)
{
    // Hash the key
    hashVal = fHash->getHashVal(key1, fHashModulus, fMemoryManager);
    assert(hashVal < fHashModulus);

    // Search that bucket for the key
    RefHash2KeysTableBucketElem<TVal>* curElem = fBucketList[hashVal];
    while (curElem)
    {
		if (key2==curElem->fKey2 && fHash->equals(key1, curElem->fKey1))
            return curElem;

        curElem = curElem->fNext;
    }
    return 0;
}

template <class TVal> const RefHash2KeysTableBucketElem<TVal>* RefHash2KeysTableOf<TVal>::
findBucketElem(const void* const key1, const int key2, unsigned int& hashVal) const
{
    // Hash the key
    hashVal = fHash->getHashVal(key1, fHashModulus, fMemoryManager);
    assert(hashVal < fHashModulus);

    // Search that bucket for the key
    const RefHash2KeysTableBucketElem<TVal>* curElem = fBucketList[hashVal];
    while (curElem)
    {
        if (fHash->equals(key1, curElem->fKey1) && (key2==curElem->fKey2))
            return curElem;

        curElem = curElem->fNext;
    }
    return 0;
}


template <class TVal> void RefHash2KeysTableOf<TVal>::
rehash()
{
    const unsigned int newMod = (fHashModulus * 8)+1;

    RefHash2KeysTableBucketElem<TVal>** newBucketList =
        (RefHash2KeysTableBucketElem<TVal>**) fMemoryManager->allocate
    (
        newMod * sizeof(RefHash2KeysTableBucketElem<TVal>*)
    );//new RefHash2KeysTableBucketElem<TVal>*[fHashModulus];

    // Make sure the new bucket list is destroyed if an
    // exception is thrown.
    ArrayJanitor<RefHash2KeysTableBucketElem<TVal>*>  guard(newBucketList, fMemoryManager);

    memset(newBucketList, 0, newMod * sizeof(newBucketList[0]));

    // Rehash all existing entries.
    for (unsigned int index = 0; index < fHashModulus; index++)
    {
        // Get the bucket list head for this entry
        RefHash2KeysTableBucketElem<TVal>* curElem = fBucketList[index];
        while (curElem)
        {
            // Save the next element before we detach this one
            RefHash2KeysTableBucketElem<TVal>* nextElem = curElem->fNext;

            const unsigned int hashVal = fHash->getHashVal(curElem->fKey1, newMod, fMemoryManager);
            assert(hashVal < newMod);
            
            RefHash2KeysTableBucketElem<TVal>* newHeadElem = newBucketList[hashVal];
            
            // Insert at the start of this bucket's list.
            curElem->fNext = newHeadElem;
            newBucketList[hashVal] = curElem;
            
            curElem = nextElem;
        }
    }
            
    RefHash2KeysTableBucketElem<TVal>** const oldBucketList = fBucketList;

    // Everything is OK at this point, so update the
    // member variables.
    fBucketList = guard.release();
    fHashModulus = newMod;

    // Delete the old bucket list.
    fMemoryManager->deallocate(oldBucketList);//delete[] oldBucketList;
    
}



// ---------------------------------------------------------------------------
//  RefHash2KeysTableOfEnumerator: Constructors and Destructor
// ---------------------------------------------------------------------------
template <class TVal> RefHash2KeysTableOfEnumerator<TVal>::
RefHash2KeysTableOfEnumerator(RefHash2KeysTableOf<TVal>* const toEnum
                              , const bool adopt
                              , MemoryManager* const manager)
	: fAdopted(adopt), fCurElem(0), fCurHash((unsigned int)-1), fToEnum(toEnum)
    , fMemoryManager(manager)
    , fLockPrimaryKey(0)
{
    if (!toEnum)
        ThrowXMLwithMemMgr(NullPointerException, XMLExcepts::CPtr_PointerIsZero, fMemoryManager);

    //
    //  Find the next available bucket element in the hash table. If it
    //  comes back zero, that just means the table is empty.
    //
    //  Note that the -1 in the current hash tells it to start from the
    //  beginning.
    //
    findNext();
}

template <class TVal> RefHash2KeysTableOfEnumerator<TVal>::~RefHash2KeysTableOfEnumerator()
{
    if (fAdopted)
        delete fToEnum;
}


// ---------------------------------------------------------------------------
//  RefHash2KeysTableOfEnumerator: Enum interface
// ---------------------------------------------------------------------------
template <class TVal> bool RefHash2KeysTableOfEnumerator<TVal>::hasMoreElements() const
{
    //
    //  If our current has is at the max and there are no more elements
    //  in the current bucket, then no more elements.
    //
    if (!fCurElem && (fCurHash == fToEnum->fHashModulus))
        return false;
    return true;
}

template <class TVal> TVal& RefHash2KeysTableOfEnumerator<TVal>::nextElement()
{
    // Make sure we have an element to return
    if (!hasMoreElements())
        ThrowXMLwithMemMgr(NoSuchElementException, XMLExcepts::Enum_NoMoreElements, fMemoryManager);

    //
    //  Save the current element, then move up to the next one for the
    //  next time around.
    //
    RefHash2KeysTableBucketElem<TVal>* saveElem = fCurElem;
    findNext();

    return *saveElem->fData;
}

template <class TVal> void RefHash2KeysTableOfEnumerator<TVal>::nextElementKey(void*& retKey1, int& retKey2)
{
    // Make sure we have an element to return
    if (!hasMoreElements())
        ThrowXMLwithMemMgr(NoSuchElementException, XMLExcepts::Enum_NoMoreElements, fMemoryManager);

    //
    //  Save the current element, then move up to the next one for the
    //  next time around.
    //
    RefHash2KeysTableBucketElem<TVal>* saveElem = fCurElem;
    findNext();

    retKey1 = saveElem->fKey1;
    retKey2 = saveElem->fKey2;

    return;
}

template <class TVal> void RefHash2KeysTableOfEnumerator<TVal>::Reset()
{
    if(fLockPrimaryKey)
        fCurHash=fToEnum->fHash->getHashVal(fLockPrimaryKey, fToEnum->fHashModulus, fMemoryManager);
    else
        fCurHash = (unsigned int)-1;
    fCurElem = 0;
    findNext();
}


template <class TVal> void RefHash2KeysTableOfEnumerator<TVal>::setPrimaryKey(const void* key)
{
    fLockPrimaryKey=key;
    Reset();
}

// ---------------------------------------------------------------------------
//  RefHash2KeysTableOfEnumerator: Private helper methods
// ---------------------------------------------------------------------------
template <class TVal> void RefHash2KeysTableOfEnumerator<TVal>::findNext()
{
    //  Code to execute if we have to return only values with the primary key
    if(fLockPrimaryKey)
    {
        if(!fCurElem)
            fCurElem = fToEnum->fBucketList[fCurHash];
        else
            fCurElem = fCurElem->fNext;
        while (fCurElem && !fToEnum->fHash->equals(fLockPrimaryKey, fCurElem->fKey1) )
            fCurElem = fCurElem->fNext;
        // if we didn't found it, make so hasMoreElements() returns false
        if(!fCurElem)
            fCurHash = fToEnum->fHashModulus;
        return;
    }
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
