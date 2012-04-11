!!ARBfp1.0
#if USE_FP_SHADOW
  OPTION ARB_fragment_program_shadow;
#endif

PARAM ambient = program.local[0];

#if DECAL
  PARAM shadingColor = program.local[1];
#endif

#if USE_SHADOW_PCF
  PARAM shadowOffsets1 = program.local[2];
  PARAM shadowOffsets2 = program.local[3];
  TEMP offset;
#endif

TEMP tex;
TEMP temp;
TEMP diffuse;
TEMP color;

#if BLEND
  // Use alpha from blend texture
  // TODO: maybe we should invert the texture instead of doing SUB here?
  TEX tex.a, fragment.texcoord[1], texture[1], 2D;
  SUB result.color.a, 1.0, tex.a;
#endif

// Load diffuse colour
TEX color, fragment.texcoord[0], texture[0], 2D;

#if DECAL
  // Use alpha from main texture
  MOV result.color.a, color;
#endif

// Compute color = texture * (ambient + diffuse*shadow)
// (diffuse is 2*fragment.color due to clamp-avoidance in the vertex program)
#if USE_SHADOW
  #if USE_FP_SHADOW
    #if USE_SHADOW_PCF
      MOV offset.zw, fragment.texcoord[2];
      ADD offset.xy, fragment.texcoord[2], shadowOffsets1;
      TEX temp.x, offset, texture[2], SHADOW2D;
      ADD offset.xy, fragment.texcoord[2], shadowOffsets1.zwzw;
      TEX temp.y, offset, texture[2], SHADOW2D;
      ADD offset.xy, fragment.texcoord[2], shadowOffsets2;
      TEX temp.z, offset, texture[2], SHADOW2D;
      ADD offset.xy, fragment.texcoord[2], shadowOffsets2.zwzw;
      TEX temp.w, offset, texture[2], SHADOW2D;
      DP4 temp, temp, 0.25;
    #else
      TEX temp, fragment.texcoord[2], texture[2], SHADOW2D;
    #endif
  #else
    TEX tex, fragment.texcoord[2], texture[2], 2D;
    MOV_SAT temp.z, fragment.texcoord[2].z;
    SGE temp, tex.x, temp.z;
  #endif
  MUL diffuse.rgb, fragment.color, 2.0;
  MAD temp.rgb, diffuse, temp, ambient;
  MUL color.rgb, color, temp;
#else
  MAD temp.rgb, fragment.color, 2.0, ambient;
  MUL color.rgb, color, temp;
#endif

// Multiply everything by the LOS texture
TEX tex.a, fragment.texcoord[3], texture[3], 2D;
MUL color.rgb, color, tex.a;

#if DECAL
  MUL result.color.rgb, color, shadingColor;
#else
  MOV result.color.rgb, color;
#endif

END
