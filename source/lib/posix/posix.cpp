#include "precompiled.h"
#include "posix.h"

#if CPU_IA32
# include "lib/sysdep/ia32/ia32.h"
#endif


#if !HAVE_C99_MATH

float rintf(float f)
{
#if CPU_IA32
	return ia32_asm_rintf(f);
#else
	return (float)(int)f;
#endif
}

double rint(double d)
{
#if CPU_IA32
	return ia32_asm_rint(d);
#else
	return (double)(int)d;
#endif
}


float fminf(float a, float b)
{
#if CPU_IA32
	return ia32_asm_fminf(a, b);
#else
	return (a < b)? a : b;
#endif
}

float fmaxf(float a, float b)
{
#if CPU_IA32
	return ia32_asm_fmaxf(a, b);
#else
	return (a > b)? a : b;
#endif
}


uint fpclassifyd(double d)
{
	// really sucky stub implementation; doesn't attempt to cover all cases.

	if(d != d)
		return FP_NAN;
	else
		return FP_NORMAL;
}

uint fpclassifyf(float f)
{
	return fpclassify((double)f);
}

#endif	// #if !HAVE_C99_MATH
