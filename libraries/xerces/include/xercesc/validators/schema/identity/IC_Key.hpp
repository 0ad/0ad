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
 * $Id: IC_Key.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(IC_KEY_HPP)
#define IC_KEY_HPP


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/schema/identity/IdentityConstraint.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT IC_Key: public IdentityConstraint
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    IC_Key(const XMLCh* const identityConstraintName,
           const XMLCh* const elemName,
           MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~IC_Key();

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    short getType() const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(IC_Key)

    IC_Key(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    IC_Key(const IC_Key& other);
    IC_Key& operator= (const IC_Key& other);
};


// ---------------------------------------------------------------------------
//  IC_Key: Getter methods
// ---------------------------------------------------------------------------
inline short IC_Key::getType() const {

    return IdentityConstraint::KEY;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file IC_Key.hpp
  */

