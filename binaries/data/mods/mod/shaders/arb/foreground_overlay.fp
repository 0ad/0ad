!!ARBfp1.0

PARAM colorMul = program.local[0];
TEMP color;
TEX color, fragment.texcoord[0], texture[0], 2D;
MUL color, color, colorMul;
MOV result.color, color;

END
