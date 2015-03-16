!!ARBfp1.0
#if USE_FP_SHADOW
  OPTION ARB_fragment_program_shadow;
#endif

PARAM ambient = program.local[0];

#if DECAL
  PARAM shadingColor = program.local[1];
#endif

#if USE_FP_SHADOW && USE_SHADOW_PCF
  PARAM shadowScale = program.local[2];
  TEMP offset, size, weight;
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

// Load diffuse color
TEX color, fragment.texcoord[0], texture[0], 2D;

#if DECAL
  // Use alpha from main texture
  MOV result.color.a, color;
#endif

// Compute color = texture * (ambient + diffuse*shadow)
// (diffuse is 2*fragment.color due to clamp-avoidance in the vertex program)
#if USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
  TEMP shadowBias;
  TEMP biasedShdw;
  MOV shadowBias.x, 0.0005;
  MOV biasedShdw, fragment.texcoord[2];
  SUB biasedShdw.z, fragment.texcoord[2].z, shadowBias.x;
  #if USE_FP_SHADOW
    #if USE_SHADOW_PCF
      SUB offset.xy, fragment.texcoord[2], 0.5;
      FRC offset.xy, offset;
      ADD size.xy, offset, 1.0;
      SUB size.zw, 2.0, offset.xyxy;

      MAD offset.xy, -0.5, offset, fragment.texcoord[2];
      MOV offset.z, biasedShdw.z;
      ADD weight, { 1.0, 1.0, -0.5, -0.5 }, offset.xyxy;
      MUL weight, weight, shadowScale.zwzw;

      MOV offset.xy, weight.zwww;
      TEX temp.x, offset, texture[2], SHADOW2D;
      MOV offset.x, weight.x;
      TEX temp.y, offset, texture[2], SHADOW2D;
      MOV offset.xy, weight.zyyy;
      TEX temp.z, offset, texture[2], SHADOW2D;
      MOV offset.x, weight.x;
      TEX temp.w, offset, texture[2], SHADOW2D;

      MUL size, size.zxzx, size.wwyy;
      DP4 temp.x, temp, size;
      MUL temp.x, temp.x, 0.111111;
    #else
      TEX temp.x, biasedShdw, texture[2], SHADOW2D;
    #endif
  #else
    TEX tex, fragment.texcoord[2], texture[2], 2D;
    MOV_SAT temp.z, biasedShdw.z;
    SGE temp.x, tex.x, temp.z;
  #endif
  MUL diffuse.rgb, fragment.color, 2.0;
  MAD temp.rgb, diffuse, temp.x, ambient;
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
