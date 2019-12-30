// This code is in the public domain -- castano@gmail.com

#pragma once
#ifndef NV_THREAD_THREADPOOL_H
#define NV_THREAD_THREADPOOL_H

#include "nvthread.h"

#include "Event.h"
#include "Thread.h"

// The thread pool creates one worker thread for each physical core. 
// The threads are idle waiting for their start events so that they do not consume any resources while inactive. 
// The thread pool runs the same function in all worker threads, the idea is to use this as the foundation of a custom task scheduler.
// When the thread pool starts, the main thread continues running, but the common use case is to inmmediately wait for the termination events of the worker threads.
// @@ The start and wait methods could probably be merged.
// It may be running the thread function on the invoking thread to avoid thread switches.

namespace nv {

    class Thread;
    class Event;

    typedef void ThreadTask(void * context, int id);

    class ThreadPool {
        NV_FORBID_COPY(ThreadPool);
    public:

        static void setup(uint workerCount, bool useThreadAffinity, bool useCallingThread);

        static ThreadPool * acquire();
        static void release(ThreadPool *);

        ThreadPool(uint workerCount = processorCount(), bool useThreadAffinity = true, bool useCallingThread = false);
        ~ThreadPool();

        void run(ThreadTask * func, void * arg);

        void start(ThreadTask * func, void * arg);
        void wait();

        //NV_THREAD_LOCAL static uint threadId;

    private:

        static void workerFunc(void * arg);

        bool useThreadAffinity;
        bool useCallingThread;
        uint workerCount;

        Thread * workers;
        Event * startEvents;
        Event * finishEvents;

        uint allIdle;

        // Current function:
        ThreadTask * func;
        void * arg;
    };


#if NV_CC_CPP11

    template <typename F>
    void thread_pool_run(F f) {
        // Transform lambda into function pointer.
        auto lambda = [](void* context, int id) {
            F & f = *reinterpret_cast<F *>(context);
            f(id);
        };

        ThreadPool * pool = ThreadPool::acquire();
        pool->run(lambda, &f);
        ThreadPool::release(pool);
    }

#endif // NV_CC_CPP11


} // namespace nv


#endif // NV_THREAD_THREADPOOL_H
