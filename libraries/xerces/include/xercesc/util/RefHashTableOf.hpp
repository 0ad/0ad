/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2003 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Log: RefHashTableOf.hpp,v $
 * Revision 1.13  2004/01/29 11:48:46  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.12  2003/12/17 00:18:35  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.11  2003/10/20 11:45:06  gareth
 * Made enumerators inherit from XMemory.
 *
 * Revision 1.10  2003/05/18 14:02:05  knoaman
 * Memory manager implementation: pass per instance manager.
 *
 * Revision 1.9  2003/05/16 21:36:59  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.8  2003/05/15 19:04:35  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.7  2003/05/15 10:37:08  gareth
 * Optimization. We now resize the hash when appropriate. Patch by Nathan Codding.
 *
 * Revision 1.6  2002/11/04 15:22:04  tng
 * C++ Namespace Support.
 *
 * Revision 1.5  2002/08/21 17:45:00  tng
 * [Bug 7087] compiler warnings when using gcc.
 *
 * Revision 1.4  2002/07/11 18:49:53  knoaman
 * Add setAdoptElements method.
 * Rename removeBucketElemSafe to orphanKey.
 *
 * Revision 1.3  2002/07/04 15:24:57  tng
 * DOM L3: add transferElement and removeBucketElemSafe for use in DOMDocument::renameNode.
 *
 * Revision 1.2  2002/06/12 17:14:03  tng
 * Add function cleanup, reinitialize and nextElementKey for ease of use.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:12  peiyongz
 * sane_include
 *
 * Revision 1.9  2001/06/04 13:45:04  tng
 * The "hash" argument clashes with STL hash.  Fixed by Pei Yong Zhang.
 *
 * Revision 1.8  2000/07/07 22:16:51  jpolast
 * remove old put(value) function.  use put(key,value) instead.
 *
 * Revision 1.7  2000/06/29 18:27:09  jpolast
 * bug fix for passing hasher class references to constructor
 *
 * Revision 1.6  2000/06/27 22:11:12  jpolast
 * added more general functionality to hashtables.
 * able to specify which hasher to use.
 * default: HashXMLCh [hashes XMLCh* strings]
 *
 * future todo: make hasher class references static so only
 * one instance of a hasher is ever created.
 *
 * Revision 1.5  2000/03/02 19:54:44  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.4  2000/02/24 20:05:25  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.3  2000/02/06 07:48:03  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.2  1999/12/18 00:18:10  roddey
 * More changes to support the new, completely orthagonal support for
 * intrinsic encodings.
 *
 * Revision 1.1.1.1  1999/11/09 01:05:01  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:12  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
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
template <class TVal> struct RefHashTableBucketElem : public XMemory
{
    RefHashTableBucketElem(void* key, TVal* const value, RefHashTableBucketElem<TVal>* next)
		: fData(value), fNext(next), fKey(key)
        {
        }

    RefHashTableBucketElem(){};

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
    void removeBucketElem(const void* const key, unsigned int& hashVal);
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

