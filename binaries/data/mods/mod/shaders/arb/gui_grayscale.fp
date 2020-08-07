!!ARBfp1.0

TEMP tex_color;
TEX tex_color, fragment.texcoord[0], texture[0], 2D;

TEMP grayscale;
MOV grayscale.r, 0.3;
MOV grayscale.g, 0.59;
MOV grayscale.b, 0.11;
MOV grayscale.a, 0.0;

TEMP color;
DP3 color.rgb, tex_color, grayscale;
MOV color.a, tex_color.a;

MOV result.color, color;

END
