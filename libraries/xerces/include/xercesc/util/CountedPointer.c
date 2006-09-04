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
 * $Id: CountedPointer.c 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#if defined(XERCES_TMPLSINC)
#include <xercesc/util/CountedPointer.hpp>
#endif

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  CountedPointerTo: Constructors and Destructor
// ---------------------------------------------------------------------------
template <class T> CountedPointerTo<T>::
CountedPointerTo(const CountedPointerTo<T>& toCopy) :

    fPtr(toCopy.fPtr)
{
    if (fPtr)
        fPtr->addRef();
}

template <class T> CountedPointerTo<T>::CountedPointerTo(T* p) :

    fPtr(p)
{
    if (fPtr)
        fPtr->addRef();
}

template <class T> CountedPointerTo<T>::~CountedPointerTo()
{
    if (fPtr)
        fPtr->removeRef();
}


// ---------------------------------------------------------------------------
//  CountedPointerTo: Operators
// ---------------------------------------------------------------------------
template <class T> CountedPointerTo<T>&
CountedPointerTo<T>::operator=(const CountedPointerTo<T>& other)
{
    if (this == &other)
        return *this;

    if (other.fPtr)
        other.fPtr->addRef();

    if (fPtr)
        fPtr->removeRef();

    fPtr = other.fPtr;
    return *this;
}

template <class T> CountedPointerTo<T>::operator T*()
{
    return fPtr;
}

template <class T> const T* CountedPointerTo<T>::operator->() const
{
    return fPtr;
}

template <class T> T* CountedPointerTo<T>::operator->()
{
    return fPtr;
}

template <class T> const T& CountedPointerTo<T>::operator*() const
{
    if (!fPtr)
        ThrowXMLwithMemMgr(NullPointerException, XMLExcepts::CPtr_PointerIsZero, 0);
    return *fPtr;
}

template <class T> T& CountedPointerTo<T>::operator*()
{
    if (!fPtr)
        ThrowXMLwithMemMgr(NullPointerException, XMLExcepts::CPtr_PointerIsZero, 0);
    return *fPtr;
}

XERCES_CPP_NAMESPACE_END
