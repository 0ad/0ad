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
 * $Log: SynchronizedStringPool.hpp,v $
 * Revision 1.1  2003/10/09 13:51:16  neilg
 * implementation of a StringPool implementation that permits thread-safe updates.  This can now be used by a grammar pool that is locked so that scanners have somehwere to store information about newly-encountered URIs
 *
 */

#if !defined(SYNCHRONIZEDSTRINGPOOL_HPP)
#define SYNCHRONIZEDSTRINGPOOL_HPP

#include <xercesc/framework/MemoryManager.hpp>
#include <xercesc/util/StringPool.hpp>
#include <xercesc/util/Mutexes.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
//  This class provides a synchronized string pool implementation.
//  This will necessarily be slower than the regular StringPool, so it
//  should only be used when updates need to be made in a thread-safe
//  way.  Updates will be made on datastructures local to this object;
//  all queries that don't involve mutation will first be directed at
//  the StringPool implementation with which this object is
//  constructed.
class XMLUTIL_EXPORT XMLSynchronizedStringPool : public XMLStringPool
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    XMLSynchronizedStringPool
    (
        const XMLStringPool *  constPool
        , const unsigned int   modulus = 109
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    virtual ~XMLSynchronizedStringPool();


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


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLSynchronizedStringPool(const XMLSynchronizedStringPool&);
    XMLSynchronizedStringPool& operator=(const XMLSynchronizedStringPool&);


    // -----------------------------------------------------------------------
    // private data members
    //  fConstPool
    //      the pool whose immutability we're protecting
    // fMutex
    //      mutex to permit synchronous updates of our StringPool
    const XMLStringPool* fConstPool;
    XMLMutex             fMutex;
};

XERCES_CPP_NAMESPACE_END

#endif
