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
 * $Id: ValueArrayOf.c 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#if defined(XERCES_TMPLSINC)
#include <xercesc/util/ValueArrayOf.hpp>
#endif


XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  ValueArrayOf: Contructors and Destructor
// ---------------------------------------------------------------------------
template <class TElem>
ValueArrayOf<TElem>::ValueArrayOf(const unsigned int size,
                                  MemoryManager* const manager) :

    fSize(size)
    , fArray(0)
    , fMemoryManager(manager)
{
    fArray = (TElem*) fMemoryManager->allocate(fSize * sizeof(TElem)); //new TElem[fSize];
}

template <class TElem>
ValueArrayOf<TElem>::ValueArrayOf( const TElem* values
                                 , const unsigned int size
                                 , MemoryManager* const manager) :

    fSize(size)
    , fArray(0)
    , fMemoryManager(manager)
{
    fArray = (TElem*) fMemoryManager->allocate(fSize * sizeof(TElem)); //new TElem[fSize];
    for (unsigned int index = 0; index < fSize; index++)
        fArray[index] = values[index];
}

template <class TElem>
ValueArrayOf<TElem>::ValueArrayOf(const ValueArrayOf<TElem>& source) :
    XMemory(source)
    , fSize(source.fSize)
    , fArray(0)
    , fMemoryManager(source.fMemoryManager)
{
    fArray = (TElem*) fMemoryManager->allocate(fSize * sizeof(TElem)); //new TElem[fSize];
    for (unsigned int index = 0; index < fSize; index++)
        fArray[index] = source.fArray[index];
}

template <class TElem> ValueArrayOf<TElem>::~ValueArrayOf()
{
    fMemoryManager->deallocate(fArray); //delete [] fArray;
}


// ---------------------------------------------------------------------------
//  ValueArrayOf: Public operators
// ---------------------------------------------------------------------------
template <class TElem> TElem& ValueArrayOf<TElem>::
operator[](const unsigned int index)
{
    if (index >= fSize)
        ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::Array_BadIndex, fMemoryManager);
    return fArray[index];
}

template <class TElem> const TElem& ValueArrayOf<TElem>::
operator[](const unsigned int index) const
{
    if (index >= fSize)
        ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::Array_BadIndex, fMemoryManager);
    return fArray[index];
}

template <class TElem> ValueArrayOf<TElem>& ValueArrayOf<TElem>::
operator=(const ValueArrayOf<TElem>& toAssign)
{
    if (this == &toAssign)
        return *this;

    // Reallocate if not the same size
    if (toAssign.fSize != fSize)
    {
        fMemoryManager->deallocate(fArray); //delete [] fArray;
        fSize = toAssign.fSize;
        fArray = (TElem*) fMemoryManager->allocate(fSize * sizeof(TElem)); //new TElem[fSize];
    }

    // Copy over the source elements
    for (unsigned int index = 0; index < fSize; index++)
        fArray[index] = toAssign.fArray[index];

    return *this;
}

template <class TElem> bool ValueArrayOf<TElem>::
operator==(const ValueArrayOf<TElem>& toCompare) const
{
    if (this == &toCompare)
        return true;

    if (fSize != toCompare.fSize)
        return false;

    for (unsigned int index = 0; index < fSize; index++)
    {
        if (fArray[index] != toCompare.fArray[index])
            return false;
    }

    return true;
}

template <class TElem> bool ValueArrayOf<TElem>::
operator!=(const ValueArrayOf<TElem>& toCompare) const
{
    return !operator==(toCompare);
}


// ---------------------------------------------------------------------------
//  ValueArrayOf: Copy operations
// ---------------------------------------------------------------------------
template <class TElem> unsigned int ValueArrayOf<TElem>::
copyFrom(const ValueArrayOf<TElem>& srcArray)
{
    //
    //  Copy over as many of the source elements as will fit into
    //  this array.
    //
    const unsigned int count = fSize < srcArray.fSize ?
                                fSize : srcArray.fSize;

    for (unsigned int index = 0; index < count; index++)
        fArray[index] = srcArray.fArray[index];

    return count;
}


// ---------------------------------------------------------------------------
//  ValueArrayOf: Getter methods
// ---------------------------------------------------------------------------
template <class TElem> unsigned int ValueArrayOf<TElem>::
length() const
{
    return fSize;
}

template <class TElem> TElem* ValueArrayOf<TElem>::
rawData() const
{
    return fArray;
}


// ---------------------------------------------------------------------------
//  ValueArrayOf: Miscellaneous methods
// ---------------------------------------------------------------------------
template <class TElem> void ValueArrayOf<TElem>::
resize(const unsigned int newSize)
{
    if (newSize == fSize)
        return;

    if (newSize < fSize)
        ThrowXMLwithMemMgr(IllegalArgumentException, XMLExcepts::Array_BadNewSize, fMemoryManager);

    // Allocate the new array
    TElem* newArray = (TElem*) fMemoryManager->allocate
    (
        newSize * sizeof(TElem)
    ); //new TElem[newSize];

    // Copy the existing values
    unsigned int index = 0;
    for (; index < fSize; index++)
        newArray[index] = fArray[index];

    for (; index < newSize; index++)
        newArray[index] = TElem(0);

    // Delete the old array and udpate our members
    fMemoryManager->deallocate(fArray); //delete [] fArray;
    fArray = newArray;
    fSize = newSize;
}



// ---------------------------------------------------------------------------
//  ValueArrayEnumerator: Constructors and Destructor
// ---------------------------------------------------------------------------
template <class TElem> ValueArrayEnumerator<TElem>::
ValueArrayEnumerator(ValueArrayOf<TElem>* const toEnum, const bool adopt) :
    fAdopted(adopt)
    , fCurIndex(0)
    , fToEnum(toEnum)
{
}

template <class TElem> ValueArrayEnumerator<TElem>::~ValueArrayEnumerator()
{
    if (fAdopted)
        delete fToEnum;
}


// ---------------------------------------------------------------------------
//  ValueArrayEnumerator: Enum interface
// ---------------------------------------------------------------------------
template <class TElem> bool ValueArrayEnumerator<TElem>::hasMoreElements() const
{
    if (fCurIndex >= fToEnum->length())
        return false;
    return true;
}

template <class TElem> TElem& ValueArrayEnumerator<TElem>::nextElement()
{
    return (*fToEnum)[fCurIndex++];
}

template <class TElem> void ValueArrayEnumerator<TElem>::Reset()
{
    fCurIndex = 0;
}

XERCES_CPP_NAMESPACE_END
