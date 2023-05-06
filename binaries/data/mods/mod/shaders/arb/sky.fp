!!ARBfp1.0
# cgc version 3.1.0013, build date Apr 24 2012
# command line args: -oglsl -profile arbfp1
# source file: sky.fs
#vendor NVIDIA Corporation
#version 3.1.0.13
#profile arbfp1
#program main
#semantic baseTex
#var float4 gl_FragColor : $vout.COLOR : COL : -1 : 1
#var samplerCUBE baseTex :  : texunit 0 : -1 : 1
#var float3 v_tex : $vin.TEX0 : TEX0 : -1 : 1
#const c[0] = 0.25 0 1 4
PARAM c[1] = { { 0.25, 0, 1, 4 } };
TEMP R0;
TEMP R1;
TEMP R2;
SLT R2.x, c[0].y, fragment.texcoord[0].y;
ABS R2.x, R2;
TEX R0, fragment.texcoord[0], texture[0], CUBE;
ADD R1.x, -fragment.texcoord[0].y, c[0];
MUL R1, R0, R1.x;
MUL R1, R1, c[0].w;
CMP R2.x, -R2, c[0].y, c[0].z;
CMP result.color, -R2.x, R0, R1;
END
# 8 instructions, 3 R-regs
