!!ARBfp1.0

ATTRIB v_tex = fragment.texcoord[0];

TEX result.color, v_tex, texture[0], 2D;

END
