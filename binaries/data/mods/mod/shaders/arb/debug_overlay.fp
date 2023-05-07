!!ARBfp1.0

#if DEBUG_TEXTURED
ATTRIB v_tex = fragment.texcoord[0];

TEX result.color, v_tex, texture[0], 2D;
#else
PARAM color = program.local[0];

MOV result.color, color;
#endif

END
