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

/*
 * $Id: RefHashTableOf.hpp 191054 2005-06-17 02:56:35Z jberry $
 */


#if !defined(REFHASHTABLEOF_HPP)
#define REFHASHTABLEOF_HPP


#include <xercesc/util/HashBase.hpp>
#include <xercesc/util/IllegalArgumentException.hpp>
#include <xercesc/util/NoSuchElementException.hpp>
#include <xercesc/util/RuntimeException.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/HashXMLCh.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemoryManager.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
//  Forward declare the enumerator so he can be our friend. Can you say
//  friend? Sure...
//
template <class TVal> class RefHashTableOfEnumerator;
template <class TVal> struct RefHashTableBucketElem;


//
//  This should really be a nested class, but some of the compilers we
//  have to support cannot deal with that!
//
template <class TVal> struct RefHashTableBucketElem
{
    RefHashTableBucketElem(void* key, TVal* const value, RefHashTableBucketElem<TVal>* next)
		: fData(value), fNext(next), fKey(key)
        {
        }

    RefHashTableBucketElem(){};
    ~RefHashTableBucketElem(){};

    TVal*                           fData;
    RefHashTableBucketElem<TVal>*   fNext;
	void*							fKey;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RefHashTableBucketElem(const RefHashTableBucketElem<TVal>&);
    RefHashTableBucketElem<TVal>& operator=(const RefHashTableBucketElem<TVal>&);
};


template <class TVal> class RefHashTableOf : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
	// backwards compatability - default hasher is HashXMLCh
    RefHashTableOf
    (
        const unsigned int modulus
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
	// backwards compatability - default hasher is HashXMLCh
    RefHashTableOf
    (
        const unsigned int modulus
        , const bool adoptElems
        , MemoryManager* const manager =  XMLPlatformUtils::fgMemoryManager
    );
	// if a hash function is passed in, it will be deleted when the hashtable is deleted.
	// use a new instance of the hasher class for each hashtable, otherwise one hashtable
	// may delete the hasher of a different hashtable if both use the same hasher.
    RefHashTableOf
    (
        const unsigned int modulus
        , const bool adoptElems
        , HashBase* hashBase
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    ~RefHashTableOf();


    // -----------------------------------------------------------------------
    //  Element management
    // -----------------------------------------------------------------------
    bool isEmpty() const;
    bool containsKey(const void* const key) const;
    void removeKey(const void* const key);
    void removeAll();
    void cleanup();
    void reinitialize(HashBase* hashBase);
    void transferElement(const void* const key1, void* key2);
    TVal* orphanKey(const void* const key);

    // -----------------------------------------------------------------------
    //  Getters
    // -----------------------------------------------------------------------
    TVal* get(const void* const key);
    const TVal* get(const void* const key) const;
    MemoryManager* getMemoryManager() const;   
    unsigned int   getHashModulus()   const;
    unsigned int   getCount() const;

    // -----------------------------------------------------------------------
    //  Setters
    // -----------------------------------------------------------------------
    void setAdoptElements(const bool aValue);


    // -----------------------------------------------------------------------
    //  Putters
    // -----------------------------------------------------------------------
	void put(void* key, TVal* const valueToAdopt);


private :
    // -----------------------------------------------------------------------
    //  Declare our friends
    // -----------------------------------------------------------------------
    friend class RefHashTableOfEnumerator<TVal>;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RefHashTableOf(const RefHashTableOf<TVal>&);
    RefHashTableOf<TVal>& operator=(const RefHashTableOf<TVal>&);

    // -----------------------------------------------------------------------
    //  Private methods
    // -----------------------------------------------------------------------
    RefHashTableBucketElem<TVal>* findBucketElem(const void* const key, unsigned int& hashVal);
    const RefHashTableBucketElem<TVal>* findBucketElem(const void* const key, unsigned int& hashVal) const;
    void initialize(const unsigned int modulus);
    void rehash();


    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fAdoptedElems
    //      Indicates whether the values added are adopted or just referenced.
    //      If adopted, then they are deleted when they are removed from the
    //      hash table.
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
    MemoryManager*                 fMemoryManager;
    bool                           fAdoptedElems;
    RefHashTableBucketElem<TVal>** fBucketList;
    unsigned int                   fHashModulus;
    unsigned int                   fInitialModulus;
    unsigned int                   fCount;
    HashBase*                      fHash;
};



//
//  An enumerator for a value array. It derives from the basic enumerator
//  class, so that value vectors can be generically enumerated.
//
template <class TVal> class RefHashTableOfEnumerator : public XMLEnumerator<TVal>, public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    RefHashTableOfEnumerator(RefHashTableOf<TVal>* const toEnum
        , const bool adopt = false
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    virtual ~RefHashTableOfEnumerator();

    RefHashTableOfEnumerator(const RefHashTableOfEnumerator<TVal>&);
    // -----------------------------------------------------------------------
    //  Enum interface
    // -----------------------------------------------------------------------
    bool hasMoreElements() const;
    TVal& nextElement();
    void Reset();

    // -----------------------------------------------------------------------
    //  New interface specific for key used in RefHashable
    // -----------------------------------------------------------------------
    void* nextElementKey();

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RefHashTableOfEnumerator<TVal>& operator=(const RefHashTableOfEnumerator<TVal>&);

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
    bool                                  fAdopted;
    RefHashTableBucketElem<TVal>*         fCurElem;
    unsigned int                          fCurHash;
    RefHashTableOf<TVal>*                 fToEnum;
    MemoryManager* const                  fMemoryManager;
};

XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/util/RefHashTableOf.c>
#endif

#endif

