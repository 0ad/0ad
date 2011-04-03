!!ARBfp1.0
TEMP tex;

TEX tex, fragment.texcoord[0], texture[0], 2D;
MUL result.color, tex, fragment.color;

END
