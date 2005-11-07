#include "precompiled.h"

#include "lib.h"
#include "sysdep.h"
#if CPU_IA32
# include "ia32.h"
#endif
#if OS_WIN
# include "win/wcpu.h"
#endif


#include <memory.h>
#include <stdarg.h>


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


// no C99, and not running on IA-32 (where this is defined to ia32_rint)
// => need to implement our fallback version.
#if !HAVE_C99 && !defined(rint)

inline float rintf(float f)
{
	return (float)(int)f;
}

inline double rint(double d)
{
	return (double)(int)d;
}

#endif


// float->int conversion: not using the ia32 version; just implement as a
// cast. (see USE_IA32_FLOAT_TO_INT definition for details)
#if !USE_IA32_FLOAT_TO_INT

i32 i32_from_float(float f)
{
	return (i32)f;
}

i32 i32_from_double(double d)
{
	return (i32)d;
}

i64 i64_from_double(double d)
{
	return (i64)d;
}

#endif


// not possible with POSIX calls.
// called from ia32.cpp get_cpu_count
int on_each_cpu(void(*cb)())
{
#if OS_WIN
	return wcpu_on_each_cpu(cb);
#else
	// apparently not possible on non-Windows OSes because they seem to lack
	// a CPU affinity API.
	return ERR_NO_SYS;
#endif
}
