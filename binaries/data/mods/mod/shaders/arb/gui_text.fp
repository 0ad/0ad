!!ARBfp1.0

TEMP tex_color;
TEX tex_color, fragment.texcoord[0], texture[0], 2D;

PARAM add_color = program.local[1];
PARAM mul_color = program.local[2];

TEMP color;
ADD color, tex_color, add_color;
MUL color, color, mul_color;

MOV result.color, color;

END
