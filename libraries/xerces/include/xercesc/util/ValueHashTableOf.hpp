/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002 The Apache Software Foundation.  All rights
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
 * $Id: ValueHashTableOf.hpp,v 1.11 2004/01/29 11:48:46 cargilld Exp $
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
template <class TVal> struct ValueHashTableBucketElem : public XMemory
{
    ValueHashTableBucketElem(void* key, const TVal& value, ValueHashTableBucketElem<TVal>* next)
		: fData(value), fNext(next), fKey(key)
        {
        }
    ValueHashTableBucketElem(){};

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
