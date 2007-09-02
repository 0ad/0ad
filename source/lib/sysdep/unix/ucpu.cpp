#include "precompiled.h"

#include "ucpu.h"

#if OS_MACOSX
#include <sys/sysctl.h>
#endif

#if OS_LINUX
#include "valgrind.h"
#endif

int ucpu_IsThrottlingPossible()
{
	return -1; // don't know
}

int ucpu_NumPackages()
{
#if OS_MACOSX
	int mib[]={CTL_HW, HW_NCPU};
	int ncpus;
	size_t len = sizeof(ncpus);
	if (sysctl(mib, 2, &ncpus, &len, NULL, 0) == -1)
		return -1; // don't know
	else
		return ncpus;
#else
	long res = sysconf(_SC_NPROCESSORS_CONF);
	if (res == -1)
		return 1;
	else
		return (int)res;
#endif
}

double ucpu_ClockFrequency()
{
	return -1; // don't know
}


#if OS_LINUX
LibError ucpu_CallByEachCPU(CpuCallback cb, void* param)
{
	long ncpus = sysconf(_SC_NPROCESSORS_CONF);

	// Valgrind reports the number of real CPUs, but only emulates a single CPU.
	// That causes problems when we expect all those CPUs to be distinct, so
	// just pretend there's only one CPU
	if (RUNNING_ON_VALGRIND)
		ncpus = 1;

	cpu_set_t set;
	for (long i = 0; i < ncpus && i < CPU_SETSIZE; ++i)
	{
		CPU_ZERO(&set);
		CPU_SET(i, &set);
		
		int ret = sched_setaffinity(0, sizeof(set), &set);
		if (ret)
			WARN_RETURN(ERR::FAIL);
		// (The process gets migrated immediately by the setaffinity call)
		
		cb(param);
	}
	return INFO::OK;
}
#else
LibError ucpu_CallByEachCPU(CpuCallback cb, void* param)
{
	// TODO: implement
	return ERR::NOT_IMPLEMENTED;
}
#endif
