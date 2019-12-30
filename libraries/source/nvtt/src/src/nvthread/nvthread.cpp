// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "nvthread.h"

#include "Thread.h"

#if NV_OS_WIN32
#include "Win32.h"
#elif NV_OS_UNIX
#include <sys/types.h>
#if !NV_OS_LINUX
#include <sys/sysctl.h>
#endif
#include <unistd.h>
#elif NV_OS_DARWIN
#import <stdio.h>
#import <string.h>
#import <mach/mach_host.h>
#import <sys/sysctl.h>

//#include <CoreFoundation/CoreFoundation.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#endif

using namespace nv;

#if NV_OS_WIN32

typedef BOOL(WINAPI *LPFN_GSI)(LPSYSTEM_INFO);
typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

static bool isWow64() {
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle("kernel32"), "IsWow64Process");

    BOOL wow64 = FALSE;

    if (NULL != fnIsWow64Process) {
        if (!fnIsWow64Process(GetCurrentProcess(), &wow64)) {
            // If error, assume false.
        }
    }

    return wow64 != 0;
}

static void getSystemInfo(SYSTEM_INFO * sysinfo) {
    BOOL success = FALSE;

    if (isWow64()) {
        LPFN_GSI fnGetNativeSystemInfo = (LPFN_GSI)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetNativeSystemInfo");

        if (fnGetNativeSystemInfo != NULL) {
            success = fnGetNativeSystemInfo(sysinfo);
        }
    }

    if (!success) {
        GetSystemInfo(sysinfo);
    }
}

#endif // NV_OS_WIN32

// Find the number of logical processors in the system.
// Based on: http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
uint nv::processorCount() {
#if NV_OS_WIN32
    SYSTEM_INFO sysinfo;
    getSystemInfo(&sysinfo);
    //return sysinfo.dwNumberOfProcessors;

    // Respect process affinity mask?
    DWORD_PTR pam, sam;
    GetProcessAffinityMask(GetCurrentProcess(), &pam, &sam);

    // Count number of bits set in the processor affinity mask.
    uint count = 0;
    for (int i = 0; i < sizeof(DWORD_PTR) * 8; i++) {
        if (pam & (DWORD_PTR(1) << i)) count++;
    }
    nvDebugCheck(count <= sysinfo.dwNumberOfProcessors);

    return count;
#elif NV_OS_ORBIS
    return 6;
#elif NV_OS_XBOX
    return 3; // or 6?
#elif NV_OS_LINUX || NV_OS_NETBSD // Linux, Solaris, & AIX
    return sysconf(_SC_NPROCESSORS_ONLN);
#elif NV_OS_DARWIN || NV_OS_FREEBSD || NV_OS_OPENBSD
    int numCPU;
    int mib[4];
    size_t len = sizeof(numCPU);

    // set the mib for hw.ncpu
    mib[0] = CTL_HW;

#if NV_OS_OPENBSD || NV_OS_FREEBSD
    mib[1] = HW_NCPU;
#else
    mib[1] = HW_AVAILCPU;
#endif

    // get the number of CPUs from the system
    sysctl(mib, 2, &numCPU, &len, NULL, 0);

    if (numCPU < 1) {
        mib[1] = HW_NCPU;
        sysctl( mib, 2, &numCPU, &len, NULL, 0 );

        if (numCPU < 1) {
            return 1; // Assume single core.
        }
    }

    return numCPU;
#else
    return 1; // Assume single core.
#endif
}


uint nv::threadId() {
#if NV_OS_WIN32
    return GetCurrentThreadId();
#else
    return 0;   // @@ 
#endif
}


// @@ If we are using less worker threads than processors and hyperthreading is available, we probably want to enumerate the logical processors 
// so that the first cores of each processor goes first. This way, if say, we leave 2 hardware threads free, then we still have one worker
// thread on each physical processor.

// I believe that currently logical processors are enumerated in physical order, that is:
//   0 = thread a in physical core 0
//   1 = thread b in physical core 0
//   2 = thread a in physical core 1
//   ... and so on ...
// I'm not sure we can actually rely on that. And in any case we should start detecting the number of physical processors, which appears to be a pain 
// to do in a way that's compatible with newer i7 processors.

void nv::lockThreadToProcessor(int idx) {
#if NV_OS_WIN32
    //nvDebugCheck(idx < hardwareThreadCount());
#if 0
    DWORD_PTR tam = 1 << idx;
#else
    DWORD_PTR pam, sam;
    BOOL rc = GetProcessAffinityMask(GetCurrentProcess(), &pam, &sam);

    // Find the idx's bit set.
    uint pidx = 0;
    DWORD_PTR tam = 0;
    for (int i = 0; i < sizeof(DWORD_PTR) * 8; i++) {
        DWORD_PTR mask = DWORD_PTR(1) << i;
        if (pam & mask) {
            if (pidx == idx) {
                tam = mask;
                break;
            }
            pidx++;
        }
    }

    nvDebugCheck(tam != 0);
#endif

    SetThreadAffinityMask(GetCurrentThread(), tam);
#else
    // @@ NOP
#endif
}


void nv::unlockThreadToProcessor() {
#if NV_OS_WIN32
    DWORD_PTR pam, sam;
    BOOL rc = GetProcessAffinityMask(GetCurrentProcess(), &pam, &sam);
    SetThreadAffinityMask(GetCurrentThread(), pam);
#else
    // @@ NOP
#endif
}

uint nv::logicalProcessorCount() {
    return processorCount();
}


#if NV_OS_WIN32

struct LOGICALPROCESSORDATA
{
    unsigned int nLargestStandardFunctionNumber;
    unsigned int nLargestExtendedFunctionNumber;
    int nLogicalProcessorCount;
    int nLocalApicId;
    int nCPUcore;
    int nProcessorId;
    int nApicIdCoreIdSize;
    int nNC;
    int nMNC;
    int nCPUCoresperProcessor;
    int nThreadsperCPUCore;
    int nProcId;
    int nCoreId;
    bool CmpLegacy;
    bool HTT;
};

#define MAX_NUMBER_OF_LOGICAL_PROCESSORS 96
#define MAX_NUMBER_OF_PHYSICAL_PROCESSORS 8
#define MAX_NUMBER_OF_IOAPICS 16
static LOGICALPROCESSORDATA LogicalProcessorMap[MAX_NUMBER_OF_LOGICAL_PROCESSORS];
static int PhysProcIds[MAX_NUMBER_OF_PHYSICAL_PROCESSORS + MAX_NUMBER_OF_IOAPICS];

static void gatherProcessorData(LOGICALPROCESSORDATA * p) {

    int CPUInfo[4] = { 0, 0, 0, 0 };
    __cpuid(CPUInfo, 0);

    p->nLargestStandardFunctionNumber = CPUInfo[0];

    // Get the information associated with each valid Id
    for (uint i = 0; i <= p->nLargestStandardFunctionNumber; ++i) {
        __cpuid(CPUInfo, i);

        // Interpret CPU feature information.
        if (i == 1) {
            // Some of the bits of LocalApicId represent the CPU core 
            // within a processor and other bits represent the processor ID. 
            p->nLocalApicId = (CPUInfo[1] >> 24) & 0xff;
            p->HTT = (CPUInfo[3] >> 28) & 0x1;
            // recalculate later after 0x80000008
            p->nLogicalProcessorCount = (CPUInfo[1] >> 16) & 0x0FF;
        }
    }

    // Calling __cpuid with 0x80000000 as the InfoType argument
    // gets the number of valid extended IDs.
    __cpuid(CPUInfo, 0x80000000);
    p->nLargestExtendedFunctionNumber = CPUInfo[0];

    // Get the information associated with each extended ID.
    for (uint i = 0x80000000; i <= p->nLargestExtendedFunctionNumber; ++i) {
        __cpuid(CPUInfo, i);
        if (i == 0x80000008) {
            p->nApicIdCoreIdSize = (CPUInfo[2] >> 12) & 0xF;
            p->nNC = (CPUInfo[2]) & 0x0FF;
        }
    }

    // MNC
    // A value of zero for ApicIdCoreIdSize indicates that MNC is derived by this
    // legacy formula: MNC = NC + 1
    // A non-zero value of ApicIdCoreIdSize means that MNC is 2^ApicIdCoreIdSize  
    if (p->nApicIdCoreIdSize) {
        p->nMNC = 2;
        for (uint j = p->nApicIdCoreIdSize - 1; j > 0; j--) {
            p->nMNC = p->nMNC * 2;
        }
    }
    else {
        p->nMNC = p->nNC + 1;
    }

    // If HTT==0, then LogicalProcessorCount is reserved, and the CPU contains 
    // one CPU core and the CPU core is single-threaded.
    // If HTT==1 and CmpLegacy==1, LogicalProcessorCount represents the number of
    // CPU cores per processor, where each CPU core is single-threaded.  If HTT==1
    // and CmpLegacy==0, then LogicalProcessorCount is the number of threads per
    // processor, which is the number of cores times the number of threads per core.
    // The number of cores is NC+1.
    p->nCPUCoresperProcessor = p->nNC + 1;
    p->nThreadsperCPUCore = (p->HTT == 0 ? 1 : (p->CmpLegacy == 1 ? 1 : p->nLogicalProcessorCount / p->nCPUCoresperProcessor ));

    // Calculate a mask for the core IDs
    uint mask = 1;
    uint numbits = 1;
    if (p->nApicIdCoreIdSize) {
        numbits = p->nApicIdCoreIdSize;
        for (uint j = p->nApicIdCoreIdSize; j > 1; j--) {
            mask = (mask << 1) + 1;
        }
    }
    p->nProcId = (p->nLocalApicId & ~mask) >> numbits;
    p->nCoreId = p->nLocalApicId & mask;
}


uint nv::physicalProcessorCount() {

    uint lpc = logicalProcessorCount();

    // Get info about each logical processor.
    for (uint i = 0; i < lpc; i++) {
        // Make sure thread doesn't change processor while we gather it's data.
        lockThreadToProcessor(i);

        gatherProcessorData(&LogicalProcessorMap[i]);
    }

    unlockThreadToProcessor();

    memset(PhysProcIds, 0, sizeof(PhysProcIds));
    for (uint i = 0; i < lpc; i++) {
        PhysProcIds[LogicalProcessorMap[i].nProcId]++;
    }

    uint pc = 0;
    for (uint i = 0; i < (MAX_NUMBER_OF_PHYSICAL_PROCESSORS + MAX_NUMBER_OF_IOAPICS); i++) {
        if (PhysProcIds[i] != 0) {
            pc++;
        }
    }

    return pc;
}

#else

uint nv::physicalProcessorCount() {
    // @@ Assume the same.
    return processorCount();
}

#endif
