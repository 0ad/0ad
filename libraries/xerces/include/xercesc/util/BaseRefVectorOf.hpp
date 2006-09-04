/*
 * Copyright 1999-2002,2004 The Apache Software Foundation.
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
#if !defined(ABSTRACTVECTOROF_HPP)
#define ABSTRACTVECTOROF_HPP

#include <xercesc/util/ArrayIndexOutOfBoundsException.hpp>
#include <xercesc/util/XMLEnumerator.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemoryManager.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/** 
 * Abstract base class for the xerces internal representation of Vector. 
 * 
 * The destructor is abstract, forcing each of RefVectorOf and
 * RefArrayVectorOf to implement their own appropriate one.
 *
 */
template <class TElem> class BaseRefVectorOf : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    BaseRefVectorOf
    (
          const unsigned int maxElems
        , const bool adoptElems = true
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    virtual ~BaseRefVectorOf();


    // -----------------------------------------------------------------------
    //  Element management
    // -----------------------------------------------------------------------
    void addElement(TElem* const toAdd);
    virtual void setElementAt(TElem* const toSet, const unsigned int setAt);
    void insertElementAt(TElem* const toInsert, const unsigned int insertAt);
    TElem* orphanElementAt(const unsigned int orphanAt);
    virtual void removeAllElements();
    virtual void removeElementAt(const unsigned int removeAt);
    virtual void removeLastElement();
    bool containsElement(const TElem* const toCheck);
    virtual void cleanup();
    void reinitialize();


    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    unsigned int curCapacity() const;
    const TElem* elementAt(const unsigned int getAt) const;
    TElem* elementAt(const unsigned int getAt);
    unsigned int size() const;
    MemoryManager* getMemoryManager() const;


    // -----------------------------------------------------------------------
    //  Miscellaneous
    // -----------------------------------------------------------------------
    void ensureExtraCapacity(const unsigned int length);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    BaseRefVectorOf(const BaseRefVectorOf<TElem>& copy);
    BaseRefVectorOf& operator=(const BaseRefVectorOf<TElem>& copy);       

protected:
    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    bool            fAdoptedElems;
    unsigned int    fCurCount;
    unsigned int    fMaxCount;
    TElem**         fElemList;
    MemoryManager*  fMemoryManager;
};


//
//  An enumerator for a vector. It derives from the basic enumerator
//  class, so that value vectors can be generically enumerated.
//
template <class TElem> class BaseRefVectorEnumerator : public XMLEnumerator<TElem>, public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    BaseRefVectorEnumerator
    (
        BaseRefVectorOf<TElem>* const   toEnum
        , const bool adopt = false
    );
    virtual ~BaseRefVectorEnumerator();

    BaseRefVectorEnumerator(const BaseRefVectorEnumerator<TElem>& copy);
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
    BaseRefVectorEnumerator& operator=(const BaseRefVectorEnumerator<TElem>& copy);    
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
    //      The reference vector being enumerated.
    // -----------------------------------------------------------------------
    bool                fAdopted;
    unsigned int        fCurIndex;
    BaseRefVectorOf<TElem>*    fToEnum;
};

XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/util/BaseRefVectorOf.c>
#endif

#endif
