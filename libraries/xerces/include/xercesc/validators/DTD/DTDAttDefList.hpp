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
 * $Log: DTDAttDefList.hpp,v $
 * Revision 1.7  2004/01/29 11:52:30  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.6  2003/11/10 21:54:51  neilg
 * implementation for new stateless means of traversing attribute definition lists
 *
 * Revision 1.5  2003/10/20 11:46:28  gareth
 * Pass in memory manager to constructors and use for creation of enumerators.
 *
 * Revision 1.4  2003/10/10 16:24:51  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.3  2002/12/06 13:27:22  tng
 * [Bug 9083] Make some classes be exportable.
 *
 * Revision 1.2  2002/11/04 14:50:40  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:43  peiyongz
 * sane_include
 *
 * Revision 1.3  2000/02/24 20:16:48  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.2  2000/02/09 21:42:37  abagchi
 * Copyright swat
 *
 * Revision 1.1.1.1  1999/11/09 01:03:28  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:39  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(DTDATTDEFLIST_HPP)
#define DTDATTDEFLIST_HPP

#include <xercesc/util/RefHashTableOf.hpp>
#include <xercesc/validators/DTD/DTDElementDecl.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
//  This is a derivative of the framework abstract class which defines the
//  interface to a list of attribute defs that belong to a particular
//  element. The scanner needs to be able to get a list of the attributes
//  that an element supports, for use during the validation process and for
//  fixed/default attribute processing.
//
//  Since each validator can store attributes differently, this abstract
//  interface allows each validator to provide an implementation of this
//  data strucure that works best for it.
//
//  For us, we just wrap the RefHashTableOf collection that the DTDElementDecl
//  class uses to store the attributes that belong to it.
//
//  This clss does not adopt the hash table, it just references it. The
//  hash table is owned by the element decl it is a member of.
//
class VALIDATORS_EXPORT DTDAttDefList : public XMLAttDefList
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    DTDAttDefList
    (
        RefHashTableOf<DTDAttDef>* const    listToUse,
        MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    ~DTDAttDefList();


    // -----------------------------------------------------------------------
    //  Implementation of the virtual interface
    // -----------------------------------------------------------------------

    /** 
     * @deprecated This method is not thread-safe.
     */
    virtual bool hasMoreElements() const;
    virtual bool isEmpty() const;
    virtual XMLAttDef* findAttDef
    (
        const   unsigned long       uriID
        , const XMLCh* const        attName
    );
    virtual const XMLAttDef* findAttDef
    (
        const   unsigned long       uriID
        , const XMLCh* const        attName
    )   const;
    virtual XMLAttDef* findAttDef
    (
        const   XMLCh* const        attURI
        , const XMLCh* const        attName
    );
    virtual const XMLAttDef* findAttDef
    (
        const   XMLCh* const        attURI
        , const XMLCh* const        attName
    )   const;

    /** 
     * @deprecated This method is not thread-safe.
     */
    virtual XMLAttDef& nextElement();

    /** 
     * @deprecated This method is not thread-safe.
     */
    virtual void Reset();

    /**
     * return total number of attributes in this list
     */
    virtual unsigned int getAttDefCount() const ;

    /**
     * return attribute at the index-th position in the list.
     */
    virtual XMLAttDef &getAttDef(unsigned int index) ;

    /**
     * return attribute at the index-th position in the list.
     */
    virtual const XMLAttDef &getAttDef(unsigned int index) const ;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(DTDAttDefList)

	DTDAttDefList(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private :

    void addAttDef(DTDAttDef *toAdd);
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    DTDAttDefList(const DTDAttDefList &);
    DTDAttDefList& operator = (const  DTDAttDefList&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fEnum
    //      This is an enerator for the list that we use to do the enumerator
    //      type methods of this class.
    //
    //  fList
    //      The list of DTDAttDef objects that represent the attributes that
    //      a particular element supports.
    //  fArray
    //      vector of pointers to the DTDAttDef objects contained in this list
    //  fSize
    //      size of fArray
    //  fCount
    //      number of DTDAttDef objects currently stored in this list
    // -----------------------------------------------------------------------
    RefHashTableOfEnumerator<DTDAttDef>*    fEnum;
    RefHashTableOf<DTDAttDef>*              fList;
    DTDAttDef**                             fArray;
    unsigned int                            fSize;
    unsigned int                            fCount;

    friend class DTDElementDecl;
};

inline void DTDAttDefList::addAttDef(DTDAttDef *toAdd)
{
    if(fCount == fSize)
    {
        // need to grow fArray
        fSize <<= 1;
        DTDAttDef** newArray = (DTDAttDef **)((getMemoryManager())->allocate( sizeof(DTDAttDef*) * fSize ));
        memcpy(newArray, fArray, fCount * sizeof(DTDAttDef *));
        (getMemoryManager())->deallocate(fArray);
        fArray = newArray;
    }
    fArray[fCount++] = toAdd;
}

XERCES_CPP_NAMESPACE_END

#endif
