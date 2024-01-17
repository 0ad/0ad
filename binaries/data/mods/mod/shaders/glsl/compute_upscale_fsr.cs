#version 430

#include "common/compute.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, inTex)
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(vec4, screenSize)
END_DRAW_UNIFORMS

STORAGE_2D(0, rgba8, outTex);

#define A_GPU 1
#define A_GLSL 1

// TODO: support 16-bit floats.
#include "ffx_a.h"

#define FSR_EASU_F 1
AF4 FsrEasuRF(AF2 p) { AF4 res = textureGather(GET_DRAW_TEXTURE_2D(inTex), p, 0); return res; }
AF4 FsrEasuGF(AF2 p) { AF4 res = textureGather(GET_DRAW_TEXTURE_2D(inTex), p, 1); return res; }
AF4 FsrEasuBF(AF2 p) { AF4 res = textureGather(GET_DRAW_TEXTURE_2D(inTex), p, 2); return res; }

#include "ffx_fsr1.h"

void CurrFilter(AU2 pos)
{
	uvec4 const0, const1, const2, const3;
	FsrEasuCon(
		const0, const1, const2, const3,
		screenSize.x, screenSize.y,
		screenSize.x, screenSize.y,
		screenSize.z, screenSize.w);

	AF3 c;
	FsrEasuF(c, pos, const0, const1, const2, const3);
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
