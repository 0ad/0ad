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

/*
 * $Id: XMLRegisterCleanup.hpp 191054 2005-06-17 02:56:35Z jberry $
 */

#if !defined(XMLREGISTERCLEANUP_HPP)
#define XMLREGISTERCLEANUP_HPP

#include <xercesc/util/Mutexes.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
//  For internal use only.
//
//  This class is used by the platform utilities class to support
//  reinitialisation of global/static data which is lazily created.
//  Since that data is widely spread out the platform utilities
//  class cannot know about them directly. So, the code that creates such
//  objects creates an registers a cleanup for the object. The platform
//  termination call will iterate the list and delete the objects.
//
//  N.B. These objects need to be statically allocated.  I couldn't think
//  of a neat way of ensuring this - can anyone else?

class XMLUTIL_EXPORT XMLRegisterCleanup
{
public :
	// The cleanup function to be called on XMLPlatformUtils::Terminate()
	typedef void (*XMLCleanupFn)();
	
	void doCleanup(); 

	// This function is called during initialisation of static data to
	// register a function to be called on XMLPlatformUtils::Terminate.
	// It gives an object that uses static data an opportunity to reset
	// such data.
	void registerCleanup(XMLCleanupFn cleanupFn);

	// This function can be called either from XMLPlatformUtils::Terminate
	// to state that the cleanup has been performed and should not be
	// performed again, or from code that you have written that determines
	// that cleanup is no longer necessary.
	void unregisterCleanup();

	// The default constructor sets a state that ensures that this object
	// will do nothing
	XMLRegisterCleanup();

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
	XMLRegisterCleanup(const XMLRegisterCleanup&);
    XMLRegisterCleanup& operator=(const XMLRegisterCleanup&);

	// This is the cleanup function to be called
	XMLCleanupFn m_cleanupFn;

	// These are list pointers to the next/prev cleanup function to be called
	XMLRegisterCleanup *m_nextCleanup, *m_prevCleanup;

	// This function reinitialises the object to the default state
	void resetCleanup();
};

XERCES_CPP_NAMESPACE_END

#endif
