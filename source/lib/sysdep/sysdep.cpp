#include "precompiled.h"

#include "lib.h"
#include "sysdep.h"
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
