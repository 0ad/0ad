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

#if !defined(REFARRAYVECTOROF_HPP)
#define REFARRAYVECTOROF_HPP

#include <xercesc/util/BaseRefVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/** 
 * Class with implementation for vectors of pointers to arrays  - implements from 
 * the Abstract class Vector
 */ 
template <class TElem> class RefArrayVectorOf : public BaseRefVectorOf<TElem> 
{
public :
    // -----------------------------------------------------------------------
    //  Constructor
    // -----------------------------------------------------------------------
    RefArrayVectorOf( const unsigned int   maxElems
                    , const bool           adoptElems = true
                    , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    // -----------------------------------------------------------------------
    //  Destructor
    // -----------------------------------------------------------------------
    ~RefArrayVectorOf();

    // -----------------------------------------------------------------------
    //  Element management
    // -----------------------------------------------------------------------
    void setElementAt(TElem* const toSet, const unsigned int setAt);
    void removeAllElements();
    void removeElementAt(const unsigned int removeAt);
    void removeLastElement();
    void cleanup();
private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RefArrayVectorOf(const RefArrayVectorOf<TElem>&);
    RefArrayVectorOf<TElem>& operator=(const RefArrayVectorOf<TElem>&);
};

XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/util/RefArrayVectorOf.c>
#endif

#endif
