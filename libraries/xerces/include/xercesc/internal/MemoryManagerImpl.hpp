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
 * $Id: MemoryManagerImpl.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */


#if !defined(MEMORYMANAGERIMPL_HPP)
#define MEMORYMANAGERIMPL_HPP

#include <xercesc/framework/MemoryManager.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
  * Configurable memory manager
  *
  * <p>This is Xerces default implementation of the memory
  *    manager interface, which will be instantiated and used
  *    in the absence of an application's memory manager.
  * </p>
  */

class XMLUTIL_EXPORT MemoryManagerImpl : public MemoryManager
{
public:

    /** @name Constructor */
    //@{

    /**
      * Default constructor
      */
    MemoryManagerImpl()
    {
    }
    //@}

    /** @name Destructor */
    //@{

    /**
      * Default destructor
      */
    virtual ~MemoryManagerImpl()
    {
    }
    //@}

    /** @name The virtual methods in MemoryManager */
    //@{

    /**
      * This method allocates requested memory.
      *
      * @param size The requested memory size
      *
      * @return A pointer to the allocated memory
      */
    virtual void* allocate(size_t size);

    /**
      * This method deallocates memory
      *
      * @param p The pointer to the allocated memory to be deleted
      */
    virtual void deallocate(void* p);

    //@}

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    MemoryManagerImpl(const MemoryManagerImpl&);
    MemoryManagerImpl& operator=(const MemoryManagerImpl&);

};

XERCES_CPP_NAMESPACE_END

#endif
