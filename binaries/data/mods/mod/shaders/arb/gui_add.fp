!!ARBfp1.0

TEMP tex_color;
TEX tex_color, fragment.texcoord[0], texture[0], 2D;

PARAM add_color = program.local[1];

TEMP color;
MOV color, add_color;
MAD color.rgb, color, tex_color.a, tex_color;
MOV color.a, tex_color.a;

MOV result.color, color;

END
