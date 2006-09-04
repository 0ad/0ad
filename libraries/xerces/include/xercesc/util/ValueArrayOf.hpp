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
 * $Id: ValueArrayOf.hpp 191054 2005-06-17 02:56:35Z jberry $
 */


#if !defined(VALUEARRAY_HPP)
#define VALUEARRAY_HPP

#include <xercesc/util/XMLEnumerator.hpp>
#include <xercesc/util/ArrayIndexOutOfBoundsException.hpp>
#include <xercesc/util/IllegalArgumentException.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemoryManager.hpp>

XERCES_CPP_NAMESPACE_BEGIN

template <class TElem> class ValueArrayOf : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Contructors and Destructor
    // -----------------------------------------------------------------------
    ValueArrayOf
    (
           const unsigned int   size
         , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
	ValueArrayOf
    (
          const TElem*         values
        , const unsigned int   size
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
	ValueArrayOf(const ValueArrayOf<TElem>& source);
	~ValueArrayOf();


    // -----------------------------------------------------------------------
    //  Public operators
    // -----------------------------------------------------------------------
	TElem& operator[](const unsigned int index);
	const TElem& operator[](const unsigned int index) const;
	ValueArrayOf<TElem>& operator=(const ValueArrayOf<TElem>& toAssign);
	bool operator==(const ValueArrayOf<TElem>& toCompare) const;
	bool operator!=(const ValueArrayOf<TElem>& toCompare) const;


    // -----------------------------------------------------------------------
    //  Copy operations
    // -----------------------------------------------------------------------
    unsigned int copyFrom(const ValueArrayOf<TElem>& srcArray);


    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
	unsigned int length() const;
	TElem* rawData() const;


    // -----------------------------------------------------------------------
    //  Miscellaneous methods
    // -----------------------------------------------------------------------
    void resize(const unsigned int newSize);


private :
    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
	unsigned int    fSize;
	TElem*          fArray;
    MemoryManager*  fMemoryManager;
};


//
//  An enumerator for a value array. It derives from the basic enumerator
//  class, so that value vectors can be generically enumerated.
//
template <class TElem> class ValueArrayEnumerator : public XMLEnumerator<TElem>, public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ValueArrayEnumerator
    (
                ValueArrayOf<TElem>* const toEnum
        , const bool                       adopt = false
    );
    virtual ~ValueArrayEnumerator();


    // -----------------------------------------------------------------------
    //  Enum interface
    // -----------------------------------------------------------------------
    bool hasMoreElements() const;
    TElem& nextElement();
    void Reset();


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------    
    ValueArrayEnumerator(const ValueArrayEnumerator<TElem>&);
    ValueArrayEnumerator<TElem>& operator=(const ValueArrayEnumerator<TElem>&);

    // -----------------------------------------------------------------------
    //  Data Members
    //
    //  fAdopted
    //      Indicates whether we have adopted the passed vector. If so then
    //      we delete the vector when we are destroyed.
    //
    //  fCurIndex
    //      This is the current index into the vector.
    //
    //  fToEnum
    //      The value array being enumerated.
    // -----------------------------------------------------------------------
    bool                    fAdopted;
    unsigned int            fCurIndex;
    ValueArrayOf<TElem>*    fToEnum;
};

XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/util/ValueArrayOf.c>
#endif

#endif
