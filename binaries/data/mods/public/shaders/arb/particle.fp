!!ARBfp1.0

ATTRIB v_los = fragment.texcoord[1];
PARAM sunColor = program.local[0];

TEMP tex, losTex, color;

TEX tex, fragment.texcoord[0], texture[0], 2D;

TEMP temp;
MOV temp, 0.5;

ADD color.rgb, fragment.color, sunColor;
MUL color.rgb, color, temp;

MUL color.rgb, color, tex;

// Multiply everything by the LOS texture
TEX losTex, v_los, texture[1], 2D;
MUL result.color.rgb, color, losTex.a;
MUL result.color.a, tex, fragment.color;

END
