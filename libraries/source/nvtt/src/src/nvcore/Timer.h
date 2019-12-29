// This code is in the public domain -- castano@gmail.com

#pragma once
#ifndef NV_CORE_TIMER_H
#define NV_CORE_TIMER_H

#include "nvcore.h"

#if NV_CC_MSVC
#include <intrin.h>
#endif

namespace nv {

#if NV_CC_MSVC
    NV_FORCEINLINE uint64 fastCpuClock() { return __rdtsc(); }
#elif NV_CC_GNUC && NV_CPU_X86
    NV_FORCEINLINE uint64 fastCpuClock() {
        uint64 val;
        __asm__ volatile (".byte 0x0f, 0x31" : "=A" (val));
        return val;
    }
#elif NV_CC_GNUC && NV_CPU_X86_64
    NV_FORCEINLINE uint64 fastCpuClock() {
        uint hi, lo;
        __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
        return uint64(lo) | (uint64(hi) << 32);
    }
#else
    NV_FORCEINLINE uint64 fastCpuClock() { return 0; }    
#endif
    
    uint64 systemClockFrequency();
    uint64 systemClock();

    class NVCORE_CLASS Timer
    {
    public:
        Timer() {}

        void start() { m_start = systemClock(); }
        void stop() { m_stop = systemClock(); }

        float elapsed() const { return float(m_stop - m_start) / systemClockFrequency(); }

    private:
        uint64 m_start;
        uint64 m_stop;
    };

} // nv namespace

#endif // NV_CORE_TIMER_H
