!!ARBfp1.0

#if MINIMAP_BASE
  TEX result.color, fragment.texcoord[0], texture[0], 2D;
#endif

#if MINIMAP_LOS
  TEMP tex;
  TEMP temp;

  TEX tex, fragment.texcoord[0], texture[0], 2D;
  MOV temp.x, 1.0;
  SUB temp.y, temp.x, tex.a;

  MOV result.color.r, 0.0;
  MOV result.color.g, 0.0;
  MOV result.color.b, 0.0;
  MOV result.color.a, temp.y;
#endif

#if MINIMAP_POINT
  MOV result.color, fragment.color;
  MOV result.color.w, 1.0;
#endif

#if MINIMAP_LINE
  MOV result.color.r, 1.0;
  MOV result.color.g, 0.3;
  MOV result.color.b, 0.3;
  MOV result.color.w, 1.0;
#endif

END