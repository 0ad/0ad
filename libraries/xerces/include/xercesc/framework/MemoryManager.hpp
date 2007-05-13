/*
 * Copyright 2003,2004 The Apache Software Foundation.
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
 * $Id: MemoryManager.hpp 191054 2005-06-17 02:56:35Z jberry $
 */


#if !defined(MEMORYMANAGER_HPP)
#define MEMORYMANAGER_HPP

#include <xercesc/util/XercesDefs.hpp>
#include <stdlib.h>


XERCES_CPP_NAMESPACE_BEGIN


/**
 *  Configurable memory manager
 *
 *  <p>This interface allows outside applications to plug in their own memory
 *  manager to be used by Xerces for memory allocation/deallocation.</p> 
 */
class XMLPARSER_EXPORT MemoryManager
{
public:
    // -----------------------------------------------------------------------
    //  Constructors are hidden, only the virtual destructor is exposed
    // -----------------------------------------------------------------------

    /** @name Destructor */
    //@{

    /**
      * Default destructor
      */
    virtual ~MemoryManager()
    {
    }
    //@}


    // -----------------------------------------------------------------------
    //  The virtual memory manager interface
    // -----------------------------------------------------------------------
    /** @name The pure virtual methods in this interface. */
    //@{

    /**
      * This method allocates requested memory.
      *
      * @param size The requested memory size
      *
      * @return A pointer to the allocated memory
      */
    virtual void* allocate(size_t size) = 0;

    /**
      * This method deallocates memory
      *
      * @param p The pointer to the allocated memory to be deleted
      */
    virtual void deallocate(void* p) = 0;

    //@}


protected :
    // -----------------------------------------------------------------------
    //  Hidden Constructors
    // -----------------------------------------------------------------------
    /** @name Constructor */
    //@{

    /**
      * Protected default constructor
      */
    MemoryManager()
    {
    }
    //@}



private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    MemoryManager(const MemoryManager&);
    MemoryManager& operator=(const MemoryManager&);
};

XERCES_CPP_NAMESPACE_END

#endif
