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
 * $Log: StringPool.hpp,v $
 * Revision 1.7  2003/10/29 16:18:41  peiyongz
 * Implement serialization/deserialization
 *
 * Revision 1.6  2003/10/09 13:49:30  neilg
 * make StringPool functions virtual so that we can implement a synchronized version of StringPool for thread-safe updates.
 *
 * Revision 1.5  2003/05/16 06:01:52  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.4  2003/05/15 19:07:45  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2003/03/07 18:11:54  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.2  2002/11/04 15:22:04  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:12  peiyongz
 * sane_include
 *
 * Revision 1.5  2001/10/22 15:43:35  tng
 * [Bug 3361] "String pool id was not legal" error in Attributes::getURI().
 *
 * Revision 1.4  2000/07/07 22:16:52  jpolast
 * remove old put(value) function.  use put(key,value) instead.
 *
 * Revision 1.3  2000/02/24 20:05:25  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.2  2000/02/06 07:48:04  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:05:11  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:15  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */

#if !defined(STRINGPOOL_HPP)
#define STRINGPOOL_HPP

#include <xercesc/util/RefHashTableOf.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
//  This class implements a string pool, in which strings can be added and
//  given a unique id by which they can be refered. It has to provide fast
//  access both mapping from a string to its id and mapping from an id to
//  its string. This requires that it provide two separate data structures.
//  The map one is a hash table for quick storage and look up by name. The
//  other is an array ordered by unique id which maps to the element in the
//  hash table.
//
//  This works because strings cannot be removed from the pool once added,
//  other than flushing it completely, and because ids are assigned
//  sequentially from 1.
//
class XMLUTIL_EXPORT XMLStringPool : public XSerializable, public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    XMLStringPool
    (
          const unsigned int   modulus = 109
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    virtual ~XMLStringPool();


    // -----------------------------------------------------------------------
    //  Pool management methods
    // -----------------------------------------------------------------------
    virtual unsigned int addOrFind(const XMLCh* const newString);
    virtual bool exists(const XMLCh* const newString) const;
    virtual bool exists(const unsigned int id) const;
    virtual void flushAll();
    virtual unsigned int getId(const XMLCh* const toFind) const;
    virtual const XMLCh* getValueForId(const unsigned int id) const;
    virtual unsigned int getStringCount() const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XMLStringPool)

    XMLStringPool(MemoryManager* const manager);

private :
    // -----------------------------------------------------------------------
    //  Private data types
    // -----------------------------------------------------------------------
    class PoolElem : public XMemory
    {
        public :
            PoolElem(const XMLCh* const string,
                     const unsigned int id,
                     MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
            ~PoolElem();

            inline const XMLCh* getKey() const { return fString; }
            void reset(const XMLCh* const string, const unsigned int id);

            unsigned int    fId;
            XMLCh*          fString;
            MemoryManager*  fMemoryManager;
    };


    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLStringPool(const XMLStringPool&);
    XMLStringPool& operator=(const XMLStringPool&);


    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------
    unsigned int addNewEntry(const XMLCh* const newString);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fIdMap
    //      This is an array of pointers to the pool elements. It is ordered
    //      by unique id, so using an id to index it gives instant access to
    //      the string of that id. This is grown as required.
    //
    //  fHashTable
    //      This is the hash table used to store and quickly access the
    //      strings.
    //
    //  fMapCapacity
    //      The current capacity of the id map. When the current id hits this
    //      value the map must must be expanded.
    //
    // -----------------------------------------------------------------------
    MemoryManager*              fMemoryManager;
    PoolElem**                  fIdMap;
    RefHashTableOf<PoolElem>*   fHashTable;
    unsigned int                fMapCapacity;

protected:
    // protected data members
    //  fCurId
    //      This is the counter used to assign unique ids. It is just bumped
    //      up one for each new string added.
    unsigned int                fCurId;
};

XERCES_CPP_NAMESPACE_END

#endif
