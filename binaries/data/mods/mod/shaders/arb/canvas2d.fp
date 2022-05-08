!!ARBfp1.0

TEMP colorTex;
TEX colorTex, fragment.texcoord[0], texture[0], 2D;

TEMP grayscale;
MOV grayscale.r, 0.3;
MOV grayscale.g, 0.59;
MOV grayscale.b, 0.11;
MOV grayscale.a, 0.0;

PARAM colorAdd = program.local[1];
PARAM colorMul = program.local[2];
PARAM grayscaleFactor = program.local[3];

TEMP colorGray;
DP3 colorGray.rgb, colorTex, grayscale;
MOV colorGray.a, colorTex.a;

TEMP color;
LRP color, grayscaleFactor.r, colorGray, colorTex;
MUL color, color, colorMul;
ADD color, color, colorAdd;

MOV result.color, color;

END
