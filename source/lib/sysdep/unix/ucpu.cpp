#include "precompiled.h"

#include "ucpu.h"

int ucpu_IsThrottlingPossible()
{
	return -1; // don't know
}

int ucpu_NumPackages()
{
	long res = sysconf(_SC_NPROCESSORS_CONF);
	if (res == -1)
		return 1;
	else
		return (int)res;
}

double ucpu_ClockFrequency()
{
	return -1; // don't know
}


size_t ucpu_PageSize()
{
	return (size_t)sysconf(_SC_PAGE_SIZE);
}

LibError ucpu_CallByEachCPU(CpuCallback cb, void* param)
{
	// TODO: implement
	return ERR::NOT_IMPLEMENTED;
}
