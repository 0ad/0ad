/*
 * Copyright 1999-2005 The Apache Software Foundation.
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
//  Include
// ---------------------------------------------------------------------------
#if defined(XERCES_TMPLSINC)
#include <xercesc/util/XMLHolder.hpp>
#endif

XERCES_CPP_NAMESPACE_BEGIN


// -----------------------------------------------------------------------
// XMLHolder:  Constructors and Destructor
// -----------------------------------------------------------------------

template<class Type>
XMLHolder<Type>::XMLHolder() :
    XMemory(),
    fInstance()
{
}

template<class Type>
XMLHolder<Type>::~XMLHolder()
{
}


template<class Type>
XMLHolder<Type>*
XMLHolder<Type>::castTo(void* handle)
{
    return (XMLHolder<Type>*)handle;
}


XERCES_CPP_NAMESPACE_END
