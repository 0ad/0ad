/*
 * Copyright 2001-2004 The Apache Software Foundation.
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
 * $Id: RefHash2KeysTableOf.hpp 191054 2005-06-17 02:56:35Z jberry $
 */


#if !defined(REFHASH2KEYSTABLEOF_HPP)
#define REFHASH2KEYSTABLEOF_HPP


#include <xercesc/util/HashBase.hpp>
#include <xercesc/util/IllegalArgumentException.hpp>
#include <xercesc/util/NoSuchElementException.hpp>
#include <xercesc/util/RuntimeException.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/HashXMLCh.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// This hash table is similar to RefHashTableOf with an additional integer as key2

//
//  Forward declare the enumerator so he can be our friend. Can you say
//  friend? Sure...
//
template <class TVal> class RefHash2KeysTableOfEnumerator;
template <class TVal> struct RefHash2KeysTableBucketElem;


//
//  This should really be a nested class, but some of the compilers we
//  have to support cannot deal with that!
//
template <class TVal> struct RefHash2KeysTableBucketElem
{
    RefHash2KeysTableBucketElem(void* key1, int key2, TVal* const value, RefHash2KeysTableBucketElem<TVal>* next)
		: fData(value), fNext(next), fKey1(key1), fKey2(key2)
        {
        }
    ~RefHash2KeysTableBucketElem() {};

    TVal*                           fData;
    RefHash2KeysTableBucketElem<TVal>*   fNext;
	void*							fKey1;
	int							fKey2;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RefHash2KeysTableBucketElem(const RefHash2KeysTableBucketElem<TVal>&);
    RefHash2KeysTableBucketElem<TVal>& operator=(const RefHash2KeysTableBucketElem<TVal>&);
};


template <class TVal> class RefHash2KeysTableOf : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
	// backwards compatability - default hasher is HashXMLCh
    RefHash2KeysTableOf
    (
        const unsigned int modulus
		, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
	// backwards compatability - default hasher is HashXMLCh
    RefHash2KeysTableOf
    (
        const unsigned int modulus
        , const bool adoptElems
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
	// if a hash function is passed in, it will be deleted when the hashtable is deleted.
	// use a new instance of the hasher class for each hashtable, otherwise one hashtable
	// may delete the hasher of a different hashtable if both use the same hasher.
    RefHash2KeysTableOf
    (
        const unsigned int modulus
        , const bool adoptElems
        , HashBase* hashBase
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    ~RefHash2KeysTableOf();


    // -----------------------------------------------------------------------
    //  Element management
    // -----------------------------------------------------------------------
    bool isEmpty() const;
    bool containsKey(const void* const key1, const int key2) const;
    void removeKey(const void* const key1, const int key2);
    void removeKey(const void* const key1);
    void removeAll();
    void transferElement(const void* const key1, void* key2);

    // -----------------------------------------------------------------------
    //  Getters
    // -----------------------------------------------------------------------
    TVal* get(const void* const key1, const int key2);
    const TVal* get(const void* const key1, const int key2) const;

    MemoryManager* getMemoryManager() const;
    unsigned int   getHashModulus()   const;

    // -----------------------------------------------------------------------
    //  Putters
    // -----------------------------------------------------------------------
	void put(void* key1, int key2, TVal* const valueToAdopt);

private :
    // -----------------------------------------------------------------------
    //  Declare our friends
    // -----------------------------------------------------------------------
    friend class RefHash2KeysTableOfEnumerator<TVal>;

    
private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RefHash2KeysTableOf(const RefHash2KeysTableOf<TVal>&);
    RefHash2KeysTableOf<TVal>& operator=(const RefHash2KeysTableOf<TVal>&);

    // -----------------------------------------------------------------------
    //  Private methods
    // -----------------------------------------------------------------------
    RefHash2KeysTableBucketElem<TVal>* findBucketElem(const void* const key1, const int key2, unsigned int& hashVal);
    const RefHash2KeysTableBucketElem<TVal>* findBucketElem(const void* const key1, const int key2, unsigned int& hashVal) const;
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
    //  fCount
    //      The number of elements currently in the map
    //
    //  fHash
    //      The hasher for the key1 data type.
    // -----------------------------------------------------------------------
    MemoryManager*                      fMemoryManager;
    bool                                fAdoptedElems;
    RefHash2KeysTableBucketElem<TVal>** fBucketList;
    unsigned int                        fHashModulus;
    unsigned int                        fCount;
    HashBase*							fHash;
};



//
//  An enumerator for a value array. It derives from the basic enumerator
//  class, so that value vectors can be generically enumerated.
//
template <class TVal> class RefHash2KeysTableOfEnumerator : public XMLEnumerator<TVal>, public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    RefHash2KeysTableOfEnumerator(RefHash2KeysTableOf<TVal>* const toEnum
        , const bool adopt = false
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    virtual ~RefHash2KeysTableOfEnumerator();


    // -----------------------------------------------------------------------
    //  Enum interface
    // -----------------------------------------------------------------------
    bool hasMoreElements() const;
    TVal& nextElement();
    void Reset();

    // -----------------------------------------------------------------------
    //  New interface 
    // -----------------------------------------------------------------------
    void nextElementKey(void*&, int&);
    void setPrimaryKey(const void* key);

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RefHash2KeysTableOfEnumerator(const RefHash2KeysTableOfEnumerator<TVal>&);
    RefHash2KeysTableOfEnumerator<TVal>& operator=(const RefHash2KeysTableOfEnumerator<TVal>&);

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
    //
    //  fLockPrimaryKey
    //      Indicates that we are requested to iterate over the secondary keys
    //      associated with the given primary key
    //
    // -----------------------------------------------------------------------
    bool                                    fAdopted;
    RefHash2KeysTableBucketElem<TVal>*      fCurElem;
    unsigned int                            fCurHash;
    RefHash2KeysTableOf<TVal>*              fToEnum;
    MemoryManager* const                    fMemoryManager;
    const void*                             fLockPrimaryKey;
};

XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/util/RefHash2KeysTableOf.c>
#endif

#endif
