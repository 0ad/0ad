!!ARBfp1.0

PARAM color = program.local[2];

ATTRIB v_coords = fragment.texcoord[0];
ATTRIB v_losCoords = fragment.texcoord[1];

TEMP diffuse;
TEX diffuse, v_coords, texture[0], 2D;
MUL diffuse, diffuse, color;

TEMP los;
TEX los, v_losCoords, texture[1], 2D;
MUL diffuse, diffuse, los.a;

MOV result.color, diffuse;
END
