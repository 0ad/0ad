/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002 The Apache Software Foundation.  All rights
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

/**
 * $Log: ValueHashTableOf.c,v $
 * Revision 1.8  2003/12/17 00:18:35  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.7  2003/12/16 18:37:14  knoaman
 * Add nextElementKey method
 *
 * Revision 1.6  2003/05/16 06:01:52  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.5  2003/05/15 19:07:46  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.4  2002/11/04 15:22:05  tng
 * C++ Namespace Support.
 *
 * Revision 1.3  2002/05/24 19:46:40  knoaman
 * Initial checkin.
 *
 */


// ---------------------------------------------------------------------------
//  Include
// ---------------------------------------------------------------------------
#if defined(XERCES_TMPLSINC)
#include <xercesc/util/ValueHashTableOf.hpp>
#endif

#include <xercesc/util/NullPointerException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  ValueHashTableOf: Constructors and Destructor
// ---------------------------------------------------------------------------
template <class TVal>
ValueHashTableOf<TVal>::ValueHashTableOf( const unsigned int modulus
                                        , HashBase* hashBase
                                        , MemoryManager* const manager)
    : fMemoryManager(manager)
    , fBucketList(0)
    , fHashModulus(modulus)
    , fHash(0)
{
	initialize(modulus);
	// set hasher
	fHash = hashBase;
}

template <class TVal>
ValueHashTableOf<TVal>::ValueHashTableOf( const unsigned int modulus
                                        , MemoryManager* const manager)
	: fMemoryManager(manager)
    , fBucketList(0)
    , fHashModulus(modulus)
    , fHash(0)
{
	initialize(modulus);

	// create default hasher
	fHash = new (fMemoryManager) HashXMLCh();
}

template <class TVal> void ValueHashTableOf<TVal>::initialize(const unsigned int modulus)
{
	if (modulus == 0)
        ThrowXMLwithMemMgr(IllegalArgumentException, XMLExcepts::HshTbl_ZeroModulus, fMemoryManager);

    // Allocate the bucket list and zero them
    fBucketList = (ValueHashTableBucketElem<TVal>**) fMemoryManager->allocate
    (
        fHashModulus * sizeof(ValueHashTableBucketElem<TVal>*)
    ); //new ValueHashTableBucketElem<TVal>*[fHashModulus];
    for (unsigned int index = 0; index < fHashModulus; index++)
        fBucketList[index] = 0;
}

template <class TVal> ValueHashTableOf<TVal>::~ValueHashTableOf()
{
    removeAll();

    // Then delete the bucket list & hasher
    fMemoryManager->deallocate(fBucketList); //delete [] fBucketList;
	delete fHash;
}


// ---------------------------------------------------------------------------
//  ValueHashTableOf: Element management
// ---------------------------------------------------------------------------
template <class TVal> bool ValueHashTableOf<TVal>::isEmpty() const
{
    // Just check the bucket list for non-empty elements
    for (unsigned int buckInd = 0; buckInd < fHashModulus; buckInd++)
    {
        if (fBucketList[buckInd] != 0)
            return false;
    }
    return true;
}

template <class TVal> bool ValueHashTableOf<TVal>::
containsKey(const void* const key) const
{
    unsigned int hashVal;
    const ValueHashTableBucketElem<TVal>* findIt = findBucketElem(key, hashVal);
    return (findIt != 0);
}

template <class TVal> void ValueHashTableOf<TVal>::
removeKey(const void* const key)
{
    unsigned int hashVal;
    removeBucketElem(key, hashVal);
}

template <class TVal> void ValueHashTableOf<TVal>::removeAll()
{
    // Clean up the buckets first
    for (unsigned int buckInd = 0; buckInd < fHashModulus; buckInd++)
    {
        // Get the bucket list head for this entry
        ValueHashTableBucketElem<TVal>* curElem = fBucketList[buckInd];
        ValueHashTableBucketElem<TVal>* nextElem;
        while (curElem)
        {
            // Save the next element before we hose this one
            nextElem = curElem->fNext;

            // delete the current element and move forward
            delete curElem;
            curElem = nextElem;
        }

        // Clean out this entry
        fBucketList[buckInd] = 0;
    }
}


// ---------------------------------------------------------------------------
//  ValueHashTableOf: Getters
// ---------------------------------------------------------------------------
template <class TVal> TVal& ValueHashTableOf<TVal>::get(const void* const key, MemoryManager* const manager)
{
    unsigned int hashVal;
    ValueHashTableBucketElem<TVal>* findIt = findBucketElem(key, hashVal);
    if (!findIt)
        ThrowXMLwithMemMgr(NoSuchElementException, XMLExcepts::HshTbl_NoSuchKeyExists, manager);

    return findIt->fData;
}

template <class TVal> const TVal& ValueHashTableOf<TVal>::
get(const void* const key) const
{
    unsigned int hashVal;
    const ValueHashTableBucketElem<TVal>* findIt = findBucketElem(key, hashVal);
    if (!findIt)
        ThrowXMLwithMemMgr(NoSuchElementException, XMLExcepts::HshTbl_NoSuchKeyExists, fMemoryManager);

    return findIt->fData;
}


// ---------------------------------------------------------------------------
//  ValueHashTableOf: Putters
// ---------------------------------------------------------------------------
template <class TVal> void ValueHashTableOf<TVal>::put(void* key, const TVal& valueToAdopt)
{
    // First see if the key exists already
    unsigned int hashVal;
    ValueHashTableBucketElem<TVal>* newBucket = findBucketElem(key, hashVal);

    //
    //  If so,then update its value. If not, then we need to add it to
    //  the right bucket
    //
    if (newBucket)
    {
        newBucket->fData = valueToAdopt;
		newBucket->fKey = key;
    }
     else
    {
        newBucket = new (fMemoryManager) ValueHashTableBucketElem<TVal>(key, valueToAdopt, fBucketList[hashVal]);
        fBucketList[hashVal] = newBucket;
    }
}



// ---------------------------------------------------------------------------
//  ValueHashTableOf: Private methods
// ---------------------------------------------------------------------------
template <class TVal> ValueHashTableBucketElem<TVal>* ValueHashTableOf<TVal>::
findBucketElem(const void* const key, unsigned int& hashVal)
{
    // Hash the key
    hashVal = fHash->getHashVal(key, fHashModulus, fMemoryManager);
    if (hashVal > fHashModulus)
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::HshTbl_BadHashFromKey, fMemoryManager);

    // Search that bucket for the key
    ValueHashTableBucketElem<TVal>* curElem = fBucketList[hashVal];
    while (curElem)
    {
		if (fHash->equals(key, curElem->fKey))
            return curElem;

        curElem = curElem->fNext;
    }
    return 0;
}

template <class TVal> const ValueHashTableBucketElem<TVal>* ValueHashTableOf<TVal>::
findBucketElem(const void* const key, unsigned int& hashVal) const
{
    // Hash the key
    hashVal = fHash->getHashVal(key, fHashModulus, fMemoryManager);
    if (hashVal > fHashModulus)
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::HshTbl_BadHashFromKey, fMemoryManager);

    // Search that bucket for the key
    const ValueHashTableBucketElem<TVal>* curElem = fBucketList[hashVal];
    while (curElem)
    {
        if (fHash->equals(key, curElem->fKey))
            return curElem;

        curElem = curElem->fNext;
    }
    return 0;
}


template <class TVal> void ValueHashTableOf<TVal>::
removeBucketElem(const void* const key, unsigned int& hashVal)
{
    // Hash the key
    hashVal = fHash->getHashVal(key, fHashModulus);
    if (hashVal > fHashModulus)
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::HshTbl_BadHashFromKey, fMemoryManager);

    //
    //  Search the given bucket for this key. Keep up with the previous
    //  element so we can patch around it.
    //
    ValueHashTableBucketElem<TVal>* curElem = fBucketList[hashVal];
    ValueHashTableBucketElem<TVal>* lastElem = 0;

    while (curElem)
    {
        if (fHash->equals(key, curElem->fKey))
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
//  ValueHashTableOfEnumerator: Constructors and Destructor
// ---------------------------------------------------------------------------
template <class TVal> ValueHashTableOfEnumerator<TVal>::
ValueHashTableOfEnumerator(ValueHashTableOf<TVal>* const toEnum
                           , const bool adopt
                           , MemoryManager* const manager)
	: fAdopted(adopt), fCurElem(0), fCurHash((unsigned int)-1), fToEnum(toEnum), fMemoryManager(manager)
{
    if (!toEnum)
        ThrowXMLwithMemMgr(NullPointerException, XMLExcepts::CPtr_PointerIsZero, manager);

    //
    //  Find the next available bucket element in the hash table. If it
    //  comes back zero, that just means the table is empty.
    //
    //  Note that the -1 in the current hash tells it to start from the
    //  beginning.
    //
    findNext();
}

template <class TVal> ValueHashTableOfEnumerator<TVal>::~ValueHashTableOfEnumerator()
{
    if (fAdopted)
        delete fToEnum;
}


// ---------------------------------------------------------------------------
//  ValueHashTableOfEnumerator: Enum interface
// ---------------------------------------------------------------------------
template <class TVal> bool ValueHashTableOfEnumerator<TVal>::hasMoreElements() const
{
    //
    //  If our current has is at the max and there are no more elements
    //  in the current bucket, then no more elements.
    //
    if (!fCurElem && (fCurHash == fToEnum->fHashModulus))
        return false;
    return true;
}

template <class TVal> TVal& ValueHashTableOfEnumerator<TVal>::nextElement()
{
    // Make sure we have an element to return
    if (!hasMoreElements())
        ThrowXMLwithMemMgr(NoSuchElementException, XMLExcepts::Enum_NoMoreElements, fMemoryManager);

    //
    //  Save the current element, then move up to the next one for the
    //  next time around.
    //
    ValueHashTableBucketElem<TVal>* saveElem = fCurElem;
    findNext();

    return saveElem->fData;
}

template <class TVal> void* ValueHashTableOfEnumerator<TVal>::nextElementKey()
{
    // Make sure we have an element to return
    if (!hasMoreElements())
        ThrowXMLwithMemMgr(NoSuchElementException, XMLExcepts::Enum_NoMoreElements, fMemoryManager);

    //
    //  Save the current element, then move up to the next one for the
    //  next time around.
    //
    ValueHashTableBucketElem<TVal>* saveElem = fCurElem;
    findNext();

    return saveElem->fKey;
}


template <class TVal> void ValueHashTableOfEnumerator<TVal>::Reset()
{
    fCurHash = (unsigned int)-1;
    fCurElem = 0;
    findNext();
}



// ---------------------------------------------------------------------------
//  ValueHashTableOfEnumerator: Private helper methods
// ---------------------------------------------------------------------------
template <class TVal> void ValueHashTableOfEnumerator<TVal>::findNext()
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
