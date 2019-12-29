// This code is in the public domain -- castano@gmail.com

#pragma once
#ifndef NV_THREAD_MUTEX_H
#define NV_THREAD_MUTEX_H

#include "nvthread.h"

#include "nvcore/Ptr.h"

namespace nv
{

    class NVTHREAD_CLASS Mutex
    {
        NV_FORBID_COPY(Mutex);
    public:
        Mutex (const char * name);
        ~Mutex ();

        void lock();
        bool tryLock();
        void unlock();

    private:
        struct Private;
        AutoPtr<Private> m;
    };


    // Templated lock that can be used with any mutex.
    template <class M>
    class Lock
    {
        NV_FORBID_COPY(Lock);
    public:

        Lock (M & m) : m_mutex (m) { m_mutex.lock(); }
        ~Lock () { m_mutex.unlock(); }

    private:
        M & m_mutex;
    };

} // nv namespace

#endif // NV_THREAD_MUTEX_H
