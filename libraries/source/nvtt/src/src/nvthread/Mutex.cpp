// This code is in the public domain -- castano@gmail.com

#include "Mutex.h"

#if NV_OS_WIN32

#include "Win32.h"

#elif NV_OS_USE_PTHREAD

#include <pthread.h>
#include <errno.h> // EBUSY

#endif // NV_OS

#if NV_USE_TELEMETRY
#include <telemetry.h>
extern HTELEMETRY tmContext;
#endif

using namespace nv;


#if NV_OS_WIN32

struct Mutex::Private {
    CRITICAL_SECTION mutex;
    const char * name;
};


Mutex::Mutex (const char * name) : m(new Private)
{
    InitializeCriticalSection(&m->mutex);
    m->name = name;
#if NV_USE_TELEMETRY
    tmLockName(tmContext, this, name);
#endif
}

Mutex::~Mutex ()
{
    DeleteCriticalSection(&m->mutex);
}

void Mutex::lock()
{
#if NV_USE_TELEMETRY
    TmU64 matcher;
    tmTryLockEx(tmContext, &matcher, 100/*0.1 ms*/, __FILE__, __LINE__, this, "blocked");
#endif
    
    EnterCriticalSection(&m->mutex);

#if NV_USE_TELEMETRY
    tmEndTryLockEx(tmContext, matcher, __FILE__, __LINE__, this, TMLR_SUCCESS);
    tmSetLockState(tmContext, this, TMLS_LOCKED, "acquired");
#endif
}

bool Mutex::tryLock()
{
#if NV_USE_TELEMETRY
    TmU64 matcher;
    tmTryLockEx(tmContext, &matcher, 100/*0.1 ms*/, __FILE__, __LINE__, this, "blocked");
    if (TryEnterCriticalSection(&m->mutex) != 0) {
        tmEndTryLockEx(tmContext, matcher, __FILE__, __LINE__, this, TMLR_SUCCESS);
        tmSetLockState(tmContext, this, TMLS_LOCKED, "acquired");
        return true;
    }
    else {
        tmEndTryLockEx(tmContext, matcher, __FILE__, __LINE__, this, TMLR_FAILED);
        return false;
    }
#else
    return TryEnterCriticalSection(&m->mutex) != 0;
#endif
}

void Mutex::unlock()
{
#if NV_USE_TELEMETRY
    tmSetLockState(tmContext, this, TMLS_RELEASED, "released");
#endif

    LeaveCriticalSection(&m->mutex);
}

#elif NV_OS_USE_PTHREAD

struct Mutex::Private {
    pthread_mutex_t mutex;
    const char * name;
};


Mutex::Mutex (const char * name) : m(new Private)
{
    int result = pthread_mutex_init(&m->mutex, NULL);
    m->name = name;
    nvDebugCheck(result == 0);
}

Mutex::~Mutex ()
{
    int result = pthread_mutex_destroy(&m->mutex);
    nvDebugCheck(result == 0);
}

void Mutex::lock()
{
    int result = pthread_mutex_lock(&m->mutex);
    nvDebugCheck(result == 0);
}

bool Mutex::tryLock()
{
    int result = pthread_mutex_trylock(&m->mutex);
    nvDebugCheck(result == 0 || result == EBUSY);
    return result == 0;
}

void Mutex::unlock()
{
    int result = pthread_mutex_unlock(&m->mutex);
    nvDebugCheck(result == 0);
}

#endif // NV_OS_UNIX
