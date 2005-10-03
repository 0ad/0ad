#include "precompiled.h"

#include "MathUtil.h"
#include "graphics/Color.h"


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

