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

/**
 * $Log: RefHash2KeysTableOf.c,v $
 * Revision 1.6  2003/12/17 00:18:35  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.5  2003/10/17 21:10:40  peiyongz
 * nextElementKey() added
 *
 * Revision 1.4  2003/05/18 14:02:05  knoaman
 * Memory manager implementation: pass per instance manager.
 *
 * Revision 1.3  2003/05/15 19:04:35  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.2  2002/11/04 15:22:04  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:12  peiyongz
 * sane_include
 *
 * Revision 1.5  2001/07/19 18:43:18  peiyongz
 * fix: detect null poiniter in enumerator's ctor.
 *
 * Revision 1.4  2001/06/04 13:45:03  tng
 * The "hash" argument clashes with STL hash.  Fixed by Pei Yong Zhang.
 *
 * Revision 1.3  2001/05/11 13:26:28  tng
 * Copyright update.
 *
 * Revision 1.2  2001/03/14 13:18:21  tng
 * typo: should use fKey1
 *
 * Revision 1.1  2001/02/27 18:24:00  tng
 * Schema: Add utility RefHash2KeysTableOf.
 *
 */


// ---------------------------------------------------------------------------
//  Include
// ---------------------------------------------------------------------------
#if defined(XERCES_TMPLSINC)
#include <xercesc/util/RefHash2KeysTableOf.hpp>
#endif

#include <xercesc/util/NullPointerException.hpp>

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
    // Just check the bucket list for non-empty elements
    for (unsigned int buckInd = 0; buckInd < fHashModulus; buckInd++)
    {
        if (fBucketList[buckInd] != 0)
            return false;
    }
    return true;
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
    unsigned int hashVal;
    removeBucketElem(key1, key2, hashVal);
}

template <class TVal> void RefHash2KeysTableOf<TVal>::removeAll()
{
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
            delete curElem;
            curElem = nextElem;
        }

        // Clean out this entry
        fBucketList[buckInd] = 0;
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
// ---------------------------------------------------------------------------
//  RefHash2KeysTableOf: Putters
// ---------------------------------------------------------------------------
template <class TVal> void RefHash2KeysTableOf<TVal>::put(void* key1, int key2, TVal* const valueToAdopt)
{
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
        newBucket = new (fMemoryManager) RefHash2KeysTableBucketElem<TVal>(key1, key2, valueToAdopt, fBucketList[hashVal]);
        fBucketList[hashVal] = newBucket;
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
    if (hashVal > fHashModulus)
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::HshTbl_BadHashFromKey, fMemoryManager);

    // Search that bucket for the key
    RefHash2KeysTableBucketElem<TVal>* curElem = fBucketList[hashVal];
    while (curElem)
    {
		if (fHash->equals(key1, curElem->fKey1) && (key2==curElem->fKey2))
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
    if (hashVal > fHashModulus)
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::HshTbl_BadHashFromKey, fMemoryManager);

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
removeBucketElem(const void* const key1, const int key2, unsigned int& hashVal)
{
    // Hash the key
    hashVal = fHash->getHashVal(key1, fHashModulus);
    if (hashVal > fHashModulus)
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::HshTbl_BadHashFromKey, fMemoryManager);

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
            delete curElem;

            return;
        }

        // Move both pointers upwards
        lastElem = curElem;
        curElem = curElem->fNext;
    }

    // We never found that key
    ThrowXMLwithMemMgr(NoSuchElementException, XMLExcepts::HshTbl_NoSuchKeyExists, fMemoryManager);
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
    fCurHash = (unsigned int)-1;
    fCurElem = 0;
    findNext();
}



// ---------------------------------------------------------------------------
//  RefHash2KeysTableOfEnumerator: Private helper methods
// ---------------------------------------------------------------------------
template <class TVal> void RefHash2KeysTableOfEnumerator<TVal>::findNext()
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
        while (true)
        {
            if (fToEnum->fBucketList[fCurHash])
                break;

            // Bump to the next hash value. If we max out return
            fCurHash++;
            if (fCurHash == fToEnum->fHashModulus)
                return;
        }
        fCurElem = fToEnum->fBucketList[fCurHash];
    }
}

XERCES_CPP_NAMESPACE_END
