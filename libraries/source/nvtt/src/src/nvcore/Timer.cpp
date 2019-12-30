// This code is in the public domain -- castano@gmail.com

#include "Timer.h"

using namespace nv;
    

#if NV_OS_WIN32

#define WINDOWS_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h> // QueryPerformanceFrequency, QueryPerformanceCounter


uint64 nv::systemClockFrequency()
{
    uint64 frequency;
    QueryPerformanceFrequency((LARGE_INTEGER*) &frequency);
    return frequency;
}

uint64 nv::systemClock()
{
    uint64 counter;
    QueryPerformanceCounter((LARGE_INTEGER*) &counter);
    return counter;
}

#else

#include <time.h> // clock

uint64 nv::systemClockFrequency()
{
    return CLOCKS_PER_SEC;
}

uint64 nv::systemClock()
{
    return clock();
}

#endif
