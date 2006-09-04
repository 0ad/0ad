/*
 * Copyright 1999-2000,2004 The Apache Software Foundation.
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

#if !defined(HASHPTR_HPP)
#define HASHPTR_HPP

#include <xercesc/util/HashBase.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * The <code>HashPtr</code> class inherits from <code>HashBase</code>.
 * This is a generic hasher class designed to hash the pointers of the
 * objects themselves.  Useful if you want to hash objects instead of
 * strings so long as the objects don't move to a different memory location.
 * See <code>HashBase</code> for more information.
 */

class XMLUTIL_EXPORT HashPtr : public HashBase
{
public:
	HashPtr();
	virtual ~HashPtr();
	virtual unsigned int getHashVal(const void *const key, unsigned int mod
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	virtual bool equals(const void *const key1, const void *const key2);
private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    HashPtr(const HashPtr&);
    HashPtr& operator=(const HashPtr&);
};

XERCES_CPP_NAMESPACE_END

#endif
