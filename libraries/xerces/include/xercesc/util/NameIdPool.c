/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2000 The Apache Software Foundation.  All rights
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
 * $Log: NameIdPool.c,v $
 * Revision 1.8  2003/12/17 00:18:35  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.7  2003/10/29 16:18:05  peiyongz
 * size() added and Reset() bug fixed
 *
 * Revision 1.6  2003/05/16 06:01:52  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.5  2003/05/15 19:04:35  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.4  2002/11/04 15:22:04  tng
 * C++ Namespace Support.
 *
 * Revision 1.3  2002/09/24 19:51:24  tng
 * Performance: use XMLString::equals instead of XMLString::compareString
 *
 * Revision 1.2  2002/05/08 19:05:29  knoaman
 * [Bug 7701] NameIdPoolEnumerator copy constructor should call base class - fix by Martin Kalen
 *
 * Revision 1.1.1.1  2002/02/01 22:22:11  peiyongz
 * sane_include
 *
 * Revision 1.3  2000/03/02 19:54:42  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.2  2000/02/06 07:48:02  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:04:47  twl
 * Initial checkin
 *
 * Revision 1.3  1999/11/08 20:45:10  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
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
            delete curElem;

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
    NameIdPoolBucketElem<TElem>* newBucket = new (fMemoryManager) NameIdPoolBucketElem<TElem>
    (
        elemToAdopt
        , fBucketList[hashVal]
    );
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

    if (hashVal > fHashModulus)
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Pool_BadHashFromKey, fMemoryManager);

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

    if (hashVal > fHashModulus)
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Pool_BadHashFromKey, fMemoryManager);

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

    fCurIndex(toCopy.fCurIndex)
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
