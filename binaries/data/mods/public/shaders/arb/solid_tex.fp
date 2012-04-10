!!ARBfp1.0
#ifdef REQUIRE_ALPHA_GEQUAL
  TEMP tex;
  TEMP temp;
  TEX tex, fragment.texcoord[0], texture[0], 2D;
  SUB temp.x, tex.a, REQUIRE_ALPHA_GEQUAL;
  KIL temp.x; // discard if < 0.0
  MOV result.color, tex;
#else
  TEX result.color, fragment.texcoord[0], texture[0], 2D;
#endif
END
