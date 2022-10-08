!!ARBfp1.0

#if MINIMAP_BASE
  TEX result.color, fragment.texcoord[0], texture[0], 2D;
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

END
