/*
 * Copyright 2002,2004 The Apache Software Foundation.
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
 * $Id: ValueHashTableOf.hpp 176205 2004-11-19 00:50:22Z cargilld $
 */


#if !defined(VALUEHASHTABLEOF_HPP)
#define VALUEHASHTABLEOF_HPP


#include <xercesc/util/HashBase.hpp>
#include <xercesc/util/IllegalArgumentException.hpp>
#include <xercesc/util/NoSuchElementException.hpp>
#include <xercesc/util/RuntimeException.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/HashXMLCh.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
//  Forward declare the enumerator so he can be our friend. Can you say
//  friend? Sure...
//
template <class TVal> class ValueHashTableOfEnumerator;
template <class TVal> struct ValueHashTableBucketElem;


//
//  This should really be a nested class, but some of the compilers we
//  have to support cannot deal with that!
//
template <class TVal> struct ValueHashTableBucketElem
{
    ValueHashTableBucketElem(void* key, const TVal& value, ValueHashTableBucketElem<TVal>* next)
		: fData(value), fNext(next), fKey(key)
        {
        }
    ValueHashTableBucketElem(){};
    ~ValueHashTableBucketElem(){};

    TVal                           fData;
    ValueHashTableBucketElem<TVal>* fNext;
	void*                          fKey;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------    
    ValueHashTableBucketElem(const ValueHashTableBucketElem<TVal>&);
    ValueHashTableBucketElem<TVal>& operator=(const ValueHashTableBucketElem<TVal>&);
};


template <class TVal> class ValueHashTableOf : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
	// backwards compatability - default hasher is HashXMLCh
    ValueHashTableOf
    (
          const unsigned int   modulus
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
	// if a hash function is passed in, it will be deleted when the hashtable is deleted.
	// use a new instance of the hasher class for each hashtable, otherwise one hashtable
	// may delete the hasher of a different hashtable if both use the same hasher.
    ValueHashTableOf
    (
          const unsigned int   modulus
        , HashBase*            hashBase
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    ~ValueHashTableOf();


    // -----------------------------------------------------------------------
    //  Element management
    // -----------------------------------------------------------------------
    bool isEmpty() const;
    bool containsKey(const void* const key) const;
    void removeKey(const void* const key);
    void removeAll();


    // -----------------------------------------------------------------------
    //  Getters
    // -----------------------------------------------------------------------
    TVal& get(const void* const key, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    const TVal& get(const void* const key) const;


    // -----------------------------------------------------------------------
    //  Putters
    // -----------------------------------------------------------------------
	void put(void* key, const TVal& valueToAdopt);


private :
    // -----------------------------------------------------------------------
    //  Declare our friends
    // -----------------------------------------------------------------------
    friend class ValueHashTableOfEnumerator<TVal>;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------    
    ValueHashTableOf(const ValueHashTableOf<TVal>&);
    ValueHashTableOf<TVal>& operator=(const ValueHashTableOf<TVal>&);

    // -----------------------------------------------------------------------
    //  Private methods
    // -----------------------------------------------------------------------
    ValueHashTableBucketElem<TVal>* findBucketElem(const void* const key, unsigned int& hashVal);
    const ValueHashTableBucketElem<TVal>* findBucketElem(const void* const key, unsigned int& hashVal) const;
    void removeBucketElem(const void* const key, unsigned int& hashVal);
	void initialize(const unsigned int modulus);


    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fBucketList
    //      This is the array that contains the heads of all of the list
    //      buckets, one for each possible hash value.
    //
    //  fHashModulus
    //      The modulus used for this hash table, to hash the keys. This is
    //      also the number of elements in the bucket list.
	//
	//  fHash
	//      The hasher for the key data type.
    // -----------------------------------------------------------------------
    MemoryManager*                   fMemoryManager;
    ValueHashTableBucketElem<TVal>** fBucketList;
    unsigned int                     fHashModulus;
	HashBase*                        fHash;
};



//
//  An enumerator for a value array. It derives from the basic enumerator
//  class, so that value vectors can be generically enumerated.
//
template <class TVal> class ValueHashTableOfEnumerator : public XMLEnumerator<TVal>, public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ValueHashTableOfEnumerator(ValueHashTableOf<TVal>* const toEnum
        , const bool adopt = false
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    virtual ~ValueHashTableOfEnumerator();


    // -----------------------------------------------------------------------
    //  Enum interface
    // -----------------------------------------------------------------------
    bool hasMoreElements() const;
    TVal& nextElement();
    void Reset();

    // -----------------------------------------------------------------------
    //  New interface specific for key used in ValueHashable
    // -----------------------------------------------------------------------
    void* nextElementKey();


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------    
    ValueHashTableOfEnumerator(const ValueHashTableOfEnumerator<TVal>&);
    ValueHashTableOfEnumerator<TVal>& operator=(const ValueHashTableOfEnumerator<TVal>&);

    // -----------------------------------------------------------------------
    //  Private methods
    // -----------------------------------------------------------------------
    void findNext();


    // -----------------------------------------------------------------------
    //  Data Members
    //
    //  fAdopted
    //      Indicates whether we have adopted the passed vector. If so then
    //      we delete the vector when we are destroyed.
    //
    //  fCurElem
    //      This is the current bucket bucket element that we are on.
    //
    //  fCurHash
    //      The is the current hash buck that we are working on. Once we hit
    //      the end of the bucket that fCurElem is in, then we have to start
    //      working this one up to the next non-empty bucket.
    //
    //  fToEnum
    //      The value array being enumerated.
    // -----------------------------------------------------------------------
    bool                            fAdopted;
    ValueHashTableBucketElem<TVal>* fCurElem;
    unsigned int                    fCurHash;
    ValueHashTableOf<TVal>*         fToEnum;
    MemoryManager* const            fMemoryManager;
};

XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/util/ValueHashTableOf.c>
#endif

#endif
