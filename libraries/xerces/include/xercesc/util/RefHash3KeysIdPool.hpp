/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 2001, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Log: RefHash3KeysIdPool.hpp,v $
 * Revision 1.9  2004/01/29 11:48:46  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.8  2003/12/17 00:18:35  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.7  2003/11/03 22:00:31  peiyongz
 * RefHashTable-like enumeration accessing added
 *
 * Revision 1.6  2003/10/29 16:17:48  peiyongz
 * size() added
 *
 * Revision 1.5  2003/05/16 06:01:52  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.4  2003/05/15 19:04:35  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2002/11/04 15:22:04  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/06/12 17:15:12  tng
 * Remove redundant include header file.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:12  peiyongz
 * sane_include
 *
 * Revision 1.4  2001/12/22 01:06:08  jasons
 * Made the destructors virtual for:
 *
 * * ~RefHash2KeysTableOfEnumerator
 * * ~RefHash3KeysIdPoolEnumerator
 *
 * This fixes bug #5514
 *
 * Revision 1.3  2001/06/04 13:45:04  tng
 * The "hash" argument clashes with STL hash.  Fixed by Pei Yong Zhang.
 *
 * Revision 1.2  2001/05/11 13:26:29  tng
 * Copyright update.
 *
 * Revision 1.1  2001/03/21 21:56:12  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
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
template <class TVal> struct RefHash3KeysTableBucketElem : public XMemory
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
