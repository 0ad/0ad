!!ARBfp1.0

#if MINIMAP_BASE
  TEX result.color, fragment.texcoord[0], texture[0], 2D;
#endif

#if MINIMAP_LOS
  TEMP tex;

  TEX tex, fragment.texcoord[0], texture[0], 2D;
  SUB tex.a, 1.0, tex.a;

  MOV result.color.r, 0.0;
  MOV result.color.g, 0.0;
  MOV result.color.b, 0.0;
  MOV result.color.a, tex.a;
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
