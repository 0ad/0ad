/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2000 The Apache Software Foundation.  All rights
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
 * $Log: ValueVectorOf.hpp,v $
 * Revision 1.8  2004/01/29 11:48:46  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.7  2003/05/29 13:26:44  knoaman
 * Fix memory leak when using deprecated dom.
 *
 * Revision 1.6  2003/05/16 21:37:00  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.5  2003/05/16 06:01:52  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.4  2003/05/15 19:07:46  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2002/11/04 15:22:05  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/08/21 17:45:00  tng
 * [Bug 7087] compiler warnings when using gcc.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:13  peiyongz
 * sane_include
 *
 * Revision 1.6  2002/01/10 17:44:49  knoaman
 * Fix for bug 5786.
 *
 * Revision 1.5  2001/08/09 15:24:37  knoaman
 * add support for <anyAttribute> declaration.
 *
 * Revision 1.4  2000/03/02 19:54:47  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.3  2000/02/24 20:05:26  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.2  2000/02/06 07:48:05  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:05:33  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:19  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(VALUEVECTOROF_HPP)
#define VALUEVECTOROF_HPP

#include <xercesc/util/ArrayIndexOutOfBoundsException.hpp>
#include <xercesc/util/XMLEnumerator.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemoryManager.hpp>

XERCES_CPP_NAMESPACE_BEGIN

template <class TElem> class ValueVectorOf : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ValueVectorOf
    (
        const unsigned int maxElems
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
        , const bool toCallDestructor = false
    );
    ValueVectorOf(const ValueVectorOf<TElem>& toCopy);
    ~ValueVectorOf();


    // -----------------------------------------------------------------------
    //  Operators
    // -----------------------------------------------------------------------
    ValueVectorOf<TElem>& operator=(const ValueVectorOf<TElem>& toAssign);


    // -----------------------------------------------------------------------
    //  Element management
    // -----------------------------------------------------------------------
    void addElement(const TElem& toAdd);
    void setElementAt(const TElem& toSet, const unsigned int setAt);
    void insertElementAt(const TElem& toInsert, const unsigned int insertAt);
    void removeElementAt(const unsigned int removeAt);
    void removeAllElements();
    bool containsElement(const TElem& toCheck, const unsigned int startIndex = 0);


    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    const TElem& elementAt(const unsigned int getAt) const;
    TElem& elementAt(const unsigned int getAt);
    unsigned int curCapacity() const;
    unsigned int size() const;
    MemoryManager* getMemoryManager() const;


    // -----------------------------------------------------------------------
    //  Miscellaneous
    // -----------------------------------------------------------------------
    void ensureExtraCapacity(const unsigned int length);
    const TElem* rawData() const;


private:
    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fCurCount
    //      The count of values current added to the vector, which may be
    //      less than the internal capacity.
    //
    //  fMaxCount
    //      The current capacity of the vector.
    //
    //  fElemList
    //      The list of elements, which is dynamically allocated to the needed
    //      size.
    // -----------------------------------------------------------------------
    bool            fCallDestructor;
    unsigned int    fCurCount;
    unsigned int    fMaxCount;
    TElem*          fElemList;
    MemoryManager*  fMemoryManager;
};


//
//  An enumerator for a value vector. It derives from the basic enumerator
//  class, so that value vectors can be generically enumerated.
//
template <class TElem> class ValueVectorEnumerator : public XMLEnumerator<TElem>, public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ValueVectorEnumerator
    (
                ValueVectorOf<TElem>* const toEnum
        , const bool                        adopt = false
    );
    virtual ~ValueVectorEnumerator();


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
    ValueVectorEnumerator(const ValueVectorEnumerator<TElem>&);
    ValueVectorEnumerator<TElem>& operator=(const ValueVectorEnumerator<TElem>&);

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
    //      The value vector being enumerated.
    // -----------------------------------------------------------------------
    bool                    fAdopted;
    unsigned int            fCurIndex;
    ValueVectorOf<TElem>*   fToEnum;
};

XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/util/ValueVectorOf.c>
#endif

#endif
