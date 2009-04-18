#include "precompiled.h"
#include "posix.h"

#if ARCH_IA32
# include "lib/sysdep/arch/ia32/ia32_asm.h"
#endif


#if !HAVE_C99_MATH

float rintf(float f)
{
#if ARCH_IA32
	return ia32_asm_rintf(f);
#else
	return (float)(int)f;
#endif
}

double rint(double d)
{
#if ARCH_IA32
	return ia32_asm_rint(d);
#else
	return (double)(int)d;
#endif
}


float fminf(float a, float b)
{
#if ARCH_IA32
	return ia32_asm_fminf(a, b);
#else
	return (a < b)? a : b;
#endif
}

float fmaxf(float a, float b)
{
#if ARCH_IA32
	return ia32_asm_fmaxf(a, b);
#else
	return (a > b)? a : b;
#endif
}


size_t fpclassifyd(double d)
{
#if ARCH_IA32
	return ia32_asm_fpclassifyd(d);
#else
	// really sucky stub implementation; doesn't attempt to cover all cases.

	if(d != d)
		return FP_NAN;
	else
		return FP_NORMAL;
#endif
}

size_t fpclassifyf(float f)
{
#if ARCH_IA32
	return ia32_asm_fpclassifyf(f);
#else
	const double d = (double)f;
	return fpclassifyd(d);
#endif
}

#endif	// #if !HAVE_C99_MATH


#if EMULATE_WCSDUP
wchar_t* wcsdup(const wchar_t* str)
{
	const size_t num_chars = wcslen(str);
	wchar_t* dst = (wchar_t*)malloc((num_chars+1)*sizeof(wchar_t));	// note: wcsdup is required to use malloc
	if(!dst)
		return 0;
	SAFE_WCSCPY(dst, str);
	return dst;
}
#endif
