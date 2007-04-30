#include "precompiled.h"
#include "posix.h"

#if CPU_IA32
# include "lib/sysdep/ia32/ia32_asm.h"
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
#if CPU_IA32
	return ia32_asm_fpclassifyd(d);
#else
	// really sucky stub implementation; doesn't attempt to cover all cases.

	if(d != d)
		return FP_NAN;
	else
		return FP_NORMAL;
#endif
}

uint fpclassifyf(float f)
{
#if CPU_IA32
	return ia32_asm_fpclassifyf(f);
#else
	const double d = (double)f;
	return fpclassifyd(d);
#endif
}

#endif	// #if !HAVE_C99_MATH
