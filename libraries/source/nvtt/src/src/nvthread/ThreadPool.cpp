// This code is in the public domain -- castano@gmail.com

#include "ThreadPool.h"
#include "Mutex.h"
#include "Thread.h"
#include "Atomic.h"

#include "nvcore/Utils.h"
#include "nvcore/StrLib.h"

#if NV_USE_TELEMETRY
#include <telemetry.h>
extern HTELEMETRY tmContext;
#endif


// Most of the time it's not necessary to protect the thread pool, but if it doesn't add a significant overhead, then it'd be safer to do it.
#define PROTECT_THREAD_POOL 1


using namespace nv;

#if PROTECT_THREAD_POOL 
Mutex s_pool_mutex("thread pool");
#endif

AutoPtr<ThreadPool> s_pool;


/*static*/ void ThreadPool::setup(uint workerCount, bool useThreadAffinity, bool useCallingThread) {
#if PROTECT_THREAD_POOL 
    Lock<Mutex> lock(s_pool_mutex);
#endif

    s_pool = new ThreadPool(workerCount, useThreadAffinity, useCallingThread);
}

/*static*/ ThreadPool * ThreadPool::acquire()
{
#if PROTECT_THREAD_POOL 
    s_pool_mutex.lock();    // @@ If same thread tries to lock twice, this should assert.
#endif

    if (s_pool == NULL) {
        ThreadPool * p = new ThreadPool;
        nvDebugCheck(s_pool == p);
    }

    return s_pool.ptr();
}

/*static*/ void ThreadPool::release(ThreadPool * pool)
{
    nvDebugCheck(pool == s_pool);

    // Make sure the threads of the pool are idle.
    s_pool->wait();

#if PROTECT_THREAD_POOL 
    s_pool_mutex.unlock();
#endif
}




/*static*/ void ThreadPool::workerFunc(void * arg) {
    uint i = U32((uintptr_t)arg); // This is OK, because workerCount should always be much smaller than 2^32

    //ThreadPool::threadId = i;

    if (s_pool->useThreadAffinity) {
        lockThreadToProcessor(s_pool->useCallingThread + i);
    }

    while(true) 
    {
        s_pool->startEvents[i].wait();

        ThreadTask * func = loadAcquirePointer(&s_pool->func);

        if (func == NULL) {
            return;
        }
        
        {
#if NV_USE_TELEMETRY
            tmZoneFiltered(tmContext, 20, TMZF_NONE, "worker");
#endif
            func(s_pool->arg, s_pool->useCallingThread + i);
        }

        s_pool->finishEvents[i].post();
    }
}


ThreadPool::ThreadPool(uint workerCount/*=processorCount()*/, bool useThreadAffinity/*=true*/, bool useCallingThread/*=false*/)
{
    s_pool = this;  // Worker threads need this to be initialized before they start.

    this->useThreadAffinity = useThreadAffinity;
    this->workerCount = workerCount;
    this->useCallingThread = useCallingThread;

    uint threadCount = workerCount - useCallingThread;

    workers = new Thread[threadCount];

    startEvents = new Event[threadCount];
    finishEvents = new Event[threadCount];

    nvCompilerWriteBarrier(); // @@ Use a memory fence?

    if (useCallingThread && useThreadAffinity) {
        lockThreadToProcessor(0);   // Calling thread always locked to processor 0.
    }

    for (uint i = 0; i < threadCount; i++) {
        StringBuilder name;
        name.format("worker %d", i);
        workers[i].setName(name.release());     // @Leak
        workers[i].start(workerFunc, (void *)i);
    }

    allIdle = true;
}

ThreadPool::~ThreadPool()
{
    // Set threads to terminate.
    start(NULL, NULL);

    // Wait until threads actually exit.
    Thread::wait(workers, workerCount - useCallingThread);

    delete [] workers;
    delete [] startEvents;
    delete [] finishEvents;
}

void ThreadPool::run(ThreadTask * func, void * arg)
{
    // Wait until threads are idle.
    wait();

    start(func, arg);

    if (useCallingThread) {
        func(arg, 0);
    }

    wait();
}

void ThreadPool::start(ThreadTask * func, void * arg)
{
    // Wait until threads are idle.
    wait();

    // Set our desired function.
    storeReleasePointer(&this->func, func);
    storeReleasePointer(&this->arg, arg);

    allIdle = false;

    // Resume threads.
    Event::post(startEvents, workerCount - useCallingThread);
}

void ThreadPool::wait()
{
    if (!allIdle)
    {
        // Wait for threads to complete.
        Event::wait(finishEvents, workerCount - useCallingThread);

        allIdle = true;
    }
}
