// This code is in the public domain -- castano@gmail.com

#include "Thread.h"

#if NV_OS_WIN32
    #include "Win32.h"
#elif NV_OS_USE_PTHREAD
    #include <pthread.h>
    #include <unistd.h> // usleep
#endif

#if NV_USE_TELEMETRY
#include <telemetry.h>
extern HTELEMETRY tmContext;
#endif


using namespace nv;

struct Thread::Private
{
#if NV_OS_WIN32
    HANDLE thread;
#elif NV_OS_USE_PTHREAD
    pthread_t thread;
#endif

    ThreadFunc * func;
    void * arg;
    const char * name;
};


#if NV_OS_WIN32

unsigned long __stdcall threadFunc(void * arg) {
    Thread::Private * thread = (Thread::Private *)arg;
    thread->func(thread->arg);
    return 0;
}

// SetThreadName implementation from msdn:
// http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx

const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void setThreadName(DWORD dwThreadID, const char* threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try
    {
        RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}


#elif NV_OS_USE_PTHREAD

extern "C" void * threadFunc(void * arg) {
    Thread::Private * thread = (Thread::Private *)arg;
    thread->func(thread->arg);
    pthread_exit(0);
}

#endif


Thread::Thread() : p(new Private)
{
    p->thread = 0;
    p->name = NULL;
}

Thread::Thread(const char * name) : p(new Private)
{
    p->thread = 0;
    p->name = name;
}

Thread::~Thread()
{
    nvDebugCheck(p->thread == 0);
}

void Thread::setName(const char * name)
{
    nvCheck(p->name == NULL);
    p->name = name;
}

void Thread::start(ThreadFunc * func, void * arg)
{
    p->func = func;
    p->arg = arg;

#if NV_OS_WIN32
    DWORD threadId;
    p->thread = CreateThread(NULL, 0, threadFunc, p.ptr(), 0, &threadId);
    //p->thread = (HANDLE)_beginthreadex (0, 0, threadFunc, p.ptr(), 0, NULL);     // @@ So that we can call CRT functions...
    nvDebugCheck(p->thread != NULL);
    if (p->name != NULL) {
        setThreadName(threadId, p->name);
    #if NV_USE_TELEMETRY
        tmThreadName(tmContext, threadId, p->name);
    #endif
    }
#elif NV_OS_ORBIS
    int ret = scePthreadCreate(&p->thread, NULL, threadFunc, p.ptr(), p->name ? p->name : "nv::Thread");
    nvDebugCheck(ret == 0);
	// use any non-system core
	scePthreadSetaffinity(p->thread, 0x3F);
    scePthreadSetprio(p->thread, (SCE_KERNEL_PRIO_FIFO_DEFAULT + SCE_KERNEL_PRIO_FIFO_HIGHEST) / 2);
#elif NV_OS_USE_PTHREAD
    int result = pthread_create(&p->thread, NULL, threadFunc, p.ptr());
    nvDebugCheck(result == 0);
#endif
}

void Thread::wait()
{
#if NV_OS_WIN32
    DWORD status = WaitForSingleObject (p->thread, INFINITE);
    nvCheck (status ==  WAIT_OBJECT_0);
    BOOL ok = CloseHandle (p->thread);
    p->thread = NULL;
    nvCheck (ok);
#elif NV_OS_USE_PTHREAD
    int result = pthread_join(p->thread, NULL);
    p->thread = 0;
    nvDebugCheck(result == 0);
#endif
}

bool Thread::isRunning () const
{
#if NV_OS_WIN32
    return p->thread != NULL;
#elif NV_OS_USE_PTHREAD
    return p->thread != 0;
#endif
}

/*static*/ void Thread::spinWait(uint count)
{
    for (uint i = 0; i < count; i++) {}
}

/*static*/ void Thread::yield()
{
#if NV_OS_WIN32
    SwitchToThread();
#elif NV_OS_USE_PTHREAD
    int result = sched_yield();
    nvDebugCheck(result == 0);
#endif
}

/*static*/ void Thread::sleep(uint ms)
{
#if NV_OS_WIN32
    Sleep(ms);
#elif NV_OS_USE_PTHREAD
    usleep(1000 * ms);
#endif
}

/*static*/ void Thread::wait(Thread * threads, uint count)
{
/*#if NV_OS_WIN32
    // @@ Is there any advantage in doing this?
    nvDebugCheck(count < MAXIMUM_WAIT_OBJECTS);

    HANDLE * handles = new HANDLE[count];
    for (uint i = 0; i < count; i++) {
        handles[i] = threads->p->thread;
    }

    DWORD result = WaitForMultipleObjects(count, handles, TRUE, INFINITE);

    for (uint i = 0; i < count; i++) {
        CloseHandle (threads->p->thread);
        threads->p->thread = 0;
    }

    delete [] handles;
#else*/
    for (uint i = 0; i < count; i++) {
        threads[i].wait();
    }
//#endif
}

