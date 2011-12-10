!!ARBfp1.0
PARAM objectColor = program.local[0];
TEMP base;
TEMP mask;
TEMP color;


// Combine base texture and color, using mask texture
TEX base, fragment.texcoord[0], texture[0], 2D;
TEX mask, fragment.texcoord[0], texture[1], 2D;
LRP color.rgb, mask, objectColor, base;

#ifdef IGNORE_LOS
  MOV result.color.rgb, color;
#else
  // Multiply RGB by LOS texture (alpha channel)
  TEMP los;
  TEX los, fragment.texcoord[1], texture[2], 2D;
  MUL result.color.rgb, color, los.a;
#endif

// Use alpha from base texture, combined with the object color alpha.
// The latter is usually 1, so this basically comes down to base.a
MUL result.color.a, objectColor.a, base.a;


END
