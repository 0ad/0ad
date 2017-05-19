/*
  Copyright (c) 2007-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef ATOMICREFCOUNT_H__
#define ATOMICREFCOUNT_H__

#include "macros.h"
#include "mutex.h"

namespace gloox
{

  namespace util
  {
    /**
     * @brief A simple implementation of a thread safe 32-bit
     *  reference count. Native functions are used where possible.
     *  When not available, a mutex is used for locking and unlocking.
     *
     * @author Daniel Bowen
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.1
     */
    class GLOOX_API AtomicRefCount
    {
      public:
        /**
         * Contructs a new atomic reference count.
         */
        AtomicRefCount();

        /**
         * Increments the reference count, and returns the new value.
         * @return The new value.
         */
        int increment();

        /**
         * Decrements the reference count, and returns the new value.
         * @return The new value.
         */
        int decrement();

        /**
         * Resets the reference count to zero.
         * @since 1.0.4
         */
        void reset();
        
    private:
        AtomicRefCount& operator=( const AtomicRefCount& );

        volatile int m_count;

        // The mutex is only used if a native function is unavailable.
        Mutex m_lock;

    };

  }

}

#endif // ATOMICREFCOUNT_H__

