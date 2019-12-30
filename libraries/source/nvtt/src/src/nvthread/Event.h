// This code is in the public domain -- castano@gmail.com

#pragma once
#ifndef NV_THREAD_EVENT_H
#define NV_THREAD_EVENT_H

#include "nvthread.h"

#include "nvcore/Ptr.h"

namespace nv
{
    // This is intended to be used by a single waiter thread.
    class NVTHREAD_CLASS Event
    {
        NV_FORBID_COPY(Event);
    public:
        Event();
        ~Event();

        void post();
        void wait();    // Wait resets the event.

        static void post(Event * events, uint count);
        static void wait(Event * events, uint count);

    private:
        struct Private;
        AutoPtr<Private> m;
    };

} // nv namespace

#endif // NV_THREAD_EVENT_H
