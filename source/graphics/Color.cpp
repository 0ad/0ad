#include "precompiled.h"

#include "graphics/Color.h"

#include "maths/MathUtil.h"
#include "graphics/SColor.h"
#include "lib/sysdep/ia32/ia32.h"

static u32 fallback_ConvertRGBColorTo4ub(const RGBColor& src)
{
	SColor4ub result;
	result.R=clamp(int(src.X*255),0,255);
	result.G=clamp(int(src.Y*255),0,255);
	result.B=clamp(int(src.Z*255),0,255);
	result.A=0xff;
	return *(u32*)&result;
}

// on IA32, this is replaced by an SSE assembly version in ia32.cpp
u32 (*ConvertRGBColorTo4ub)(const RGBColor& src) = fallback_ConvertRGBColorTo4ub;


// Assembler-optimized function for color conversion
#if CPU_IA32
EXTERN_C u32 sse_ConvertRGBColorTo4ub(const RGBColor& src);
#endif

void ColorActivateFastImpl()
{
	if(0)
	{
	}
#if CPU_IA32
	else if (ia32_cap(IA32_CAP_SSE))
	{
		ConvertRGBColorTo4ub = sse_ConvertRGBColorTo4ub;
	}
#endif
	else
	{
		debug_printf("No SSE available. Slow fallback routines will be used.\n");
	}
}
