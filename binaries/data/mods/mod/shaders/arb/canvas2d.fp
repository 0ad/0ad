!!ARBfp1.0

TEMP colorTex;
TEX colorTex, fragment.texcoord[0], texture[0], 2D;

PARAM colorAdd = program.local[1];
PARAM colorMul = program.local[2];

TEMP color;
MUL color, colorTex, colorMul;
ADD color, color, colorAdd;

MOV result.color, color;

END
