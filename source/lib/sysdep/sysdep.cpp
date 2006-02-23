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

// note: stupid VC7 gets arguments wrong when using __declspec(naked);
// we need to use DWORD PTR and esp-relative addressing.

#if HAVE_MS_ASM
__declspec(naked) float fminf(float, float)
{
	__asm
	{
		fld		DWORD PTR [esp+4]
		fld		DWORD PTR [esp+8]
		fcomi	st(0), st(1)
		fcmovnb	st(0), st(1)
		fxch
		fstp	st(0)
		ret
	}
}
#else
float fminf(float a, float b)
{
	return (a < b)? a : b;
}
#endif

#if HAVE_MS_ASM
__declspec(naked) float fmaxf(float, float)
{
	__asm
	{
		fld		DWORD PTR [esp+4]
		fld		DWORD PTR [esp+8]
		fcomi	st(0), st(1)
		fcmovb	st(0), st(1)
		fxch
		fstp	st(0)
		ret
	}
}
#else
float fmaxf(float a, float b)
{
	return (a > b)? a : b;
}
#endif

#endif	// #if !HAVE_C99


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
