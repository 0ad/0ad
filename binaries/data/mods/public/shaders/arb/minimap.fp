!!ARBfp1.0

#if MINIMAP_MASK
  ATTRIB v_maskUV = fragment.texcoord[1];
  TEMP mask;
  TEX mask, v_maskUV, texture[2], 2D;
#endif

#if MINIMAP_BASE
#if MINIMAP_MASK
  TEMP color;
  TEX color, fragment.texcoord[0], texture[0], 2D;
  MUL color.a, color.a, mask.a;
  MOV result.color, color;
#else
  TEX result.color, fragment.texcoord[0], texture[0], 2D;
#endif
#endif

#if MINIMAP_LOS
  TEMP tex;

  TEX tex, fragment.texcoord[0], texture[0], 2D;

  MOV result.color.r, tex.r;
  MOV result.color.g, tex.r;
  MOV result.color.b, tex.r;
  MOV result.color.a, tex.r;
#endif

#if MINIMAP_POINT
  MOV result.color, fragment.color;
  MOV result.color.a, 1.0;
#endif

#if MINIMAP_LINE
  PARAM color = program.local[1];
  MOV result.color, color;
#endif

END
