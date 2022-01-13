!!ARBfp1.0

ATTRIB v_los = fragment.texcoord[0];
PARAM delta = program.local[0];

TEMP los1_tex;
TEMP los2_tex;

TEX los1_tex, v_los, texture[0], 2D;
TEX los2_tex, v_los, texture[1], 2D;

TEMP smoothing;
MOV_SAT smoothing, delta.x;

LRP result.color.a, smoothing, los1_tex.a, los2_tex.a;

END
