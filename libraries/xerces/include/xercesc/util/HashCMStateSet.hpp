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
 * $Id: HashCMStateSet.hpp 191054 2005-06-17 02:56:35Z jberry $
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
