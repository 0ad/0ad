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
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: HashCMStateSet.hpp,v 1.4 2004/01/29 11:48:46 cargilld Exp $
 * $Log: HashCMStateSet.hpp,v $
 * Revision 1.4  2004/01/29 11:48:46  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.3  2003/12/17 00:18:35  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.2  2002/11/04 15:22:03  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:10  peiyongz
 * sane_include
 *
 * Revision 1.2  2001/11/22 20:23:00  peiyongz
 * _declspec(dllimport) and inline warning C4273
 *
 * Revision 1.1  2001/08/16 21:54:01  peiyongz
 * new class creation
 *
 */

#if !defined(HASH_CMSTATESET_HPP)
#define HASH_CMSTATESET_HPP

#include <xercesc/util/HashBase.hpp>
#include <xercesc/validators/common/CMStateSet.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * The <code>HashCMStateSet</code> class inherits from <code>HashBase</code>.
 * This is a CMStateSet specific hasher class designed to hash the values
 * of CMStateSet.
 *
 * See <code>HashBase</code> for more information.
 */

class XMLUTIL_EXPORT HashCMStateSet : public HashBase
{
public:
	HashCMStateSet();
	virtual ~HashCMStateSet();
	virtual unsigned int getHashVal(const void *const key, unsigned int mod
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	virtual bool equals(const void *const key1, const void *const key2);
private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    HashCMStateSet(const HashCMStateSet&);
    HashCMStateSet& operator=(const HashCMStateSet&);
};

inline HashCMStateSet::HashCMStateSet()
{
}

inline HashCMStateSet::~HashCMStateSet()
{
}

inline unsigned int HashCMStateSet::getHashVal(const void *const key, unsigned int mod
                                               , MemoryManager* const)
{
    const CMStateSet* const pkey = (const CMStateSet* const) key;
	return ((pkey->hashCode()) % mod);
}

inline bool HashCMStateSet::equals(const void *const key1, const void *const key2)
{
    const CMStateSet* const pkey1 = (const CMStateSet* const) key1;
    const CMStateSet* const pkey2 = (const CMStateSet* const) key2;

	return (*pkey1==*pkey2);
}

XERCES_CPP_NAMESPACE_END

#endif
