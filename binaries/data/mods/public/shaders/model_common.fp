!!ARBfp1.0
#ifdef USE_FP_SHADOW
  OPTION ARB_fragment_program_shadow;
#endif

#ifdef LIGHTING_MODEL_old
  #define CLAMP_LIGHTING
#endif

#ifdef CLAMP_LIGHTING // for compat with old scenarios that expect clamped lighting
  #define MAD_MAYBE_SAT MAD_SAT
#else
  #define MAD_MAYBE_SAT MAD
#endif

#ifdef USE_OBJECTCOLOR
  PARAM objectColor = program.local[0];
#endif

PARAM shadingColor = program.local[1];
PARAM ambient = program.local[2];

TEMP tex;
TEMP temp;
TEMP diffuse;
TEMP color;

TEX tex, fragment.texcoord[0], texture[0], 2D;
#ifdef USE_TRANSPARENT
  MOV result.color.a, tex;
#endif

// Apply player-coloring based on texture alpha
#ifdef USE_OBJECTCOLOR
  LRP temp.rgb, objectColor, 1.0, tex.a;
  MUL color.rgb, tex, temp;
#else
  MOV color.rgb, tex;
#endif

// Compute color = texture * (ambient + diffuse*shadow)
// (diffuse is 2*fragment.color due to clamp-avoidance in the vertex program)
#ifdef USE_SHADOW
  #ifdef USE_FP_SHADOW
    TEX temp, fragment.texcoord[1], texture[1], SHADOW2D;
  #else
    TEX tex, fragment.texcoord[1], texture[1], 2D;
    MOV_SAT temp.z, fragment.texcoord[1].z;
    SGE temp, tex.x, temp.z;
  #endif
  MUL diffuse.rgb, fragment.color, 2.0;
  MAD_MAYBE_SAT temp.rgb, diffuse, temp, ambient;
  MUL color.rgb, color, temp;
#else
  MAD_MAYBE_SAT temp.rgb, fragment.color, 2.0, ambient;
  MUL color.rgb, color, temp;
#endif

// Multiply everything by the LOS texture
TEX tex.a, fragment.texcoord[2], texture[2], 2D;
MUL color.rgb, color, tex.a;

MUL result.color.rgb, color, shadingColor;

END
