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
 * $Id: XMLDeleterFor.hpp 191054 2005-06-17 02:56:35Z jberry $
 */


#if !defined(XMLDELETERFOR_HPP)
#define XMLDELETERFOR_HPP

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
//  For internal use only.
//
//  This class is used by the platform utilities class to support cleanup
//  of global/static data which is lazily created. Since that data is
//  widely spread out, and in higher level DLLs, the platform utilities
//  class cannot know about them directly. So, the code that creates such
//  objects creates an registers a deleter for the object. The platform
//  termination call will iterate the list and delete the objects.
//
template <class T> class XMLDeleterFor : public XMLDeleter
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    XMLDeleterFor(T* const toDelete);
    ~XMLDeleterFor();


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLDeleterFor();
    XMLDeleterFor(const XMLDeleterFor<T>&);
    XMLDeleterFor<T>& operator=(const XMLDeleterFor<T>&);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fToDelete
    //      This is a pointer to the data to destroy
    // -----------------------------------------------------------------------
    T*  fToDelete;
};

XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/util/XMLDeleterFor.c>
#endif

#endif
