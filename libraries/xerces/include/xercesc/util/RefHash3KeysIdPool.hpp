/*
 * Copyright 2001,2004 The Apache Software Foundation.
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
 * $Id: RefHash3KeysIdPool.hpp 191054 2005-06-17 02:56:35Z jberry $
 */


#if !defined(REFHASH3KEYSIDPOOL_HPP)
#define REFHASH3KEYSIDPOOL_HPP


#include <xercesc/util/HashBase.hpp>
#include <xercesc/util/IllegalArgumentException.hpp>
#include <xercesc/util/NoSuchElementException.hpp>
#include <xercesc/util/RuntimeException.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/HashXMLCh.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// This hash table is a combination of RefHash2KeyTableOf (with an additional integer as key3)
// and NameIdPool with an id as index

//
//  Forward declare the enumerator so he can be our friend. Can you say
//  friend? Sure...
//
template <class TVal> class RefHash3KeysIdPoolEnumerator;
template <class TVal> struct RefHash3KeysTableBucketElem;


//
//  This should really be a nested class, but some of the compilers we
//  have to support cannot deal with that!
//
template <class TVal> struct RefHash3KeysTableBucketElem
{
    RefHash3KeysTableBucketElem(
              void* key1
              , int key2
              , int key3
              , TVal* const value
              , RefHash3KeysTableBucketElem<TVal>* next) :
		fData(value)
    , fNext(next)
    , fKey1(key1)
    , fKey2(key2)
    , fKey3(key3)
    {
    }
    
    RefHash3KeysTableBucketElem() {};
    ~RefHash3KeysTableBucketElem() {};

    TVal*  fData;
    RefHash3KeysTableBucketElem<TVal>*   fNext;
    void*  fKey1;
    int    fKey2;
    int    fKey3;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RefHash3KeysTableBucketElem(const RefHash3KeysTableBucketElem<TVal>&);
    RefHash3KeysTableBucketElem<TVal>& operator=(const RefHash3KeysTableBucketElem<TVal>&);
};


template <class TVal> class RefHash3KeysIdPool : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    // backwards compatability - default hasher is HashXMLCh
    RefHash3KeysIdPool
    (
          const unsigned int   modulus
        , const unsigned int   initSize = 128
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    // backwards compatability - default hasher is HashXMLCh
    RefHash3KeysIdPool
    (
          const unsigned int   modulus
        , const bool           adoptElems
        , const unsigned int   initSize = 128
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    // if a hash function is passed in, it will be deleted when the hashtable is deleted.
    // use a new instance of the hasher class for each hashtable, otherwise one hashtable
    // may delete the hasher of a different hashtable if both use the same hasher.
    RefHash3KeysIdPool
    (
          const unsigned int   modulus
        , const bool           adoptElems
        , HashBase* hashBase
        , const unsigned int initSize = 128
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    ~RefHash3KeysIdPool();

    // -----------------------------------------------------------------------
    //  Element management
    // -----------------------------------------------------------------------
    bool isEmpty() const;
    bool containsKey(const void* const key1, const int key2, const int key3) const;
    void removeAll();


    // -----------------------------------------------------------------------
    //  Getters
    // -----------------------------------------------------------------------
    TVal* getByKey(const void* const key1, const int key2, const int key3);
    const TVal* getByKey(const void* const key1, const int key2, const int key3) const;

    TVal* getById(const unsigned elemId);
    const TVal* getById(const unsigned elemId) const;

    MemoryManager* getMemoryManager() const;
    unsigned int   getHashModulus()   const;

    // -----------------------------------------------------------------------
    //  Putters
    // -----------------------------------------------------------------------
	unsigned int put(void* key1, int key2, int key3, TVal* const valueToAdopt);


private :
    // -----------------------------------------------------------------------
    //  Declare our friends
    // -----------------------------------------------------------------------
    friend class RefHash3KeysIdPoolEnumerator<TVal>;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RefHash3KeysIdPool(const RefHash3KeysIdPool<TVal>&);
    RefHash3KeysIdPool<TVal>& operator=(const RefHash3KeysIdPool<TVal>&);

    // -----------------------------------------------------------------------
    //  Private methods
    // -----------------------------------------------------------------------
    RefHash3KeysTableBucketElem<TVal>* findBucketElem(const void* const key1, const int key2, const int key3, unsigned int& hashVal);
    const RefHash3KeysTableBucketElem<TVal>* findBucketElem(const void* const key1, const int key2, const int key3, unsigned int& hashVal) const;
    void initialize(const unsigned int modulus);


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
    //      The hasher for the key1 data type.
    //
    //  fIdPtrs
    //  fIdPtrsCount
    //      This is the array of pointers to the bucket elements in order of
    //      their assigned ids. So taking id N and referencing this array
    //      gives you the element with that id. The count field indicates
    //      the current size of this list. When fIdCounter+1 reaches this
    //      value the list must be expanded.
    //
    //  fIdCounter
    //      This is used to give out unique ids to added elements. It starts
    //      at zero (which means empty), and is bumped up for each newly added
    //      element. So the first element is 1, the next is 2, etc... This
    //      means that this value is set to the top index of the fIdPtrs array.
    // -----------------------------------------------------------------------
    MemoryManager*                      fMemoryManager;
    bool                                fAdoptedElems;
    RefHash3KeysTableBucketElem<TVal>** fBucketList;
    unsigned int                        fHashModulus;
    HashBase*                           fHash;
    TVal**                              fIdPtrs;
    unsigned int                        fIdPtrsCount;
    unsigned int                        fIdCounter;
};



//
//  An enumerator for a value array. It derives from the basic enumerator
//  class, so that value vectors can be generically enumerated.
//
template <class TVal> class RefHash3KeysIdPoolEnumerator : public XMLEnumerator<TVal>, public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    RefHash3KeysIdPoolEnumerator(RefHash3KeysIdPool<TVal>* const toEnum
        , const bool adopt = false
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    virtual ~RefHash3KeysIdPoolEnumerator();

    RefHash3KeysIdPoolEnumerator(const RefHash3KeysIdPoolEnumerator<TVal>&);
    // -----------------------------------------------------------------------
    //  Enum interface
    // -----------------------------------------------------------------------
    bool hasMoreElements() const;
    TVal& nextElement();
    void Reset();
    int  size() const;

    // -----------------------------------------------------------------------
    //  New interface 
    // -----------------------------------------------------------------------
    void resetKey();
    void nextElementKey(void*&, int&, int&);
    bool hasMoreKeys()   const;

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------    
    RefHash3KeysIdPoolEnumerator<TVal>& operator=(const RefHash3KeysIdPoolEnumerator<TVal>&);

    // -----------------------------------------------------------------------
    //  Private methods
    // -----------------------------------------------------------------------
    void findNext();

    // -----------------------------------------------------------------------
    //  Data Members
    //  fAdoptedElems
    //      Indicates whether the values added are adopted or just referenced.
    //      If adopted, then they are deleted when they are removed from the
    //      hash table
    //
    //  fCurIndex
    //      This is the current index into the pool's id mapping array. This
    //      is now we enumerate it.
    //
    //  fToEnum
    //      The name id pool that is being enumerated.
    // -----------------------------------------------------------------------
    bool                                fAdoptedElems;
    unsigned int                        fCurIndex;
    RefHash3KeysIdPool<TVal>*           fToEnum;
    RefHash3KeysTableBucketElem<TVal>*  fCurElem;
    unsigned int                        fCurHash;
    MemoryManager* const                fMemoryManager;
};

XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/util/RefHash3KeysIdPool.c>
#endif

#endif
