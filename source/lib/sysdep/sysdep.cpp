#include "precompiled.h"

#include "lib.h"
#include "sysdep.h"
#if CPU_IA32
# include "ia32.h"
#endif


#include <memory.h>
#include <stdarg.h>

#if MSC_VERSION

double round(double x)
{
	return (long)(x + 0.5);
}

#endif


#if !HAVE_C99

float fminf(float a, float b)
{
	return (a < b)? a : b;
}

float fmaxf(float a, float b)
{
	return (a > b)? a : b;
}

#endif


void memcpy2(void* dst, const void* src, size_t nbytes)
{
#if CPU_IA32
	ia32_memcpy(dst, src, nbytes);
#else
	memcpy(dst, src, nbytes);
#endif
}