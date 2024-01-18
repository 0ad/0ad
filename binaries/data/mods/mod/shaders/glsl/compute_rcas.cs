#version 430

#include "common/compute.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, inTex)
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(float, sharpness)
END_DRAW_UNIFORMS

STORAGE_2D(0, rgba8, outTex);

#define A_GPU 1
#define A_GLSL 1
#define FSR_RCAS_DENOISE 1

// TODO: support 16-bit floats.
#include "ffx_a.h"

#define FSR_RCAS_F 1
AF4 FsrRcasLoadF(ASU2 p) { return texelFetch(GET_DRAW_TEXTURE_2D(inTex), ASU2(p), 0); }
void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}

#include "ffx_fsr1.h"

void CurrFilter(AU2 pos)
{
	AU4 const0;
	FsrRcasCon(const0, sharpness);

	AF3 c;
	FsrRcasF(c.r, c.g, c.b, pos, const0);
	imageStore(outTex, ASU2(pos), AF4(c, 1));
}

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main()
{
	// Do remapping of local xy in workgroup for a more PS-like swizzle pattern.
	AU2 gxy = ARmp8x8(gl_LocalInvocationID.x) + AU2(gl_WorkGroupID.x << 4u, gl_WorkGroupID.y << 4u);
	CurrFilter(gxy);
	gxy.x += 8u;
	CurrFilter(gxy);
	gxy.y += 8u;
	CurrFilter(gxy);
	gxy.x -= 8u;
	CurrFilter(gxy);
}
