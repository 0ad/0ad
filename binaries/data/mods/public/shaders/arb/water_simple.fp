!!ARBfp1.0

PARAM color = program.local[2];

ATTRIB v_coords = fragment.texcoord[0];
ATTRIB v_losCoords = fragment.texcoord[1];

TEMP diffuse;
TEX diffuse, v_coords, texture[0], 2D;
MUL diffuse, diffuse, color;

TEMP los;
TEX los, v_losCoords, texture[1], 2D;
SUB los.r, los.r, 0.03;
MUL los.r, los.r, 0.97;
MUL diffuse, diffuse, los.r;

MOV result.color, diffuse;
END
