#include "precompiled.h"

#include "ucpu.h"

int ucpu_isThrottlingPossible()
{
	return -1; // don't know
}

int ucpu_numPackages()
{
	long res = sysconf(_SC_NPROCESSORS_CONF);
	if (res == -1)
		return 1;
	else
		return (int)res;
}

double ucpu_clockFrequency()
{
	return -1; // don't know
}


// apparently not possible on non-Windows OSes because they seem to lack
// a CPU affinity API. see sysdep.h comment.
LibError ucpu_callByEachCPU(CpuCallback cb, void* param)
{
	UNUSED2(cb);

/*
cpu_set_t currentCPU; 

while ( j < sysNumProcs ) 

{ 

CPU_ZERO(&currentCPU); 

CPU_SET(j, &currentCPU); 

if ( sched_setaffinity (0, sizeof (currentCPU), &currentCPU) 

== 0 ) 

{ 

sleep(0); // Ensure system to switch to the right
*/

	return ERR::NO_SYS;
}
