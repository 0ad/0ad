!!ARBfp1.0
PARAM objectColor = program.local[0];
TEMP base;
TEMP mask;
TEMP color;
TEMP los;

// Combine base texture and color, using mask texture
TEX base, fragment.texcoord[0], texture[0], 2D;
TEX mask, fragment.texcoord[0], texture[1], 2D;
LRP color.rgb, mask, objectColor, base;

// Multiply by LOS texture
TEX los, fragment.texcoord[1], texture[2], 2D;
MUL result.color.rgb, color, los.a;

// Use alpha from base texture
MUL result.color.a, objectColor.a, base.a;

END
