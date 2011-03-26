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

PARAM ambient = program.local[0];

TEMP tex;
TEMP temp;
TEMP diffuse;
TEMP color;

#ifdef BLEND
  // Use alpha from blend texture
  // TODO: maybe we should invert the texture instead of doing SUB here?
  TEX tex.a, fragment.texcoord[1], texture[1], 2D;
  SUB result.color.a, 1.0, tex.a;
#endif

// Load diffuse colour
TEX color, fragment.texcoord[0], texture[0], 2D;

#ifdef DECAL
  // Use alpha from main texture
  MOV result.color.a, color;
#endif

// Compute color = texture * (ambient + diffuse*shadow)
// (diffuse is 2*fragment.color due to clamp-avoidance in the vertex program)
#ifdef USE_SHADOW
  #ifdef USE_FP_SHADOW
    TEX temp, fragment.texcoord[2], texture[2], SHADOW2D;
  #else
    TEX tex, fragment.texcoord[2], texture[2], 2D;
    MOV_SAT temp.z, fragment.texcoord[2].z;
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
TEX tex.a, fragment.texcoord[3], texture[3], 2D;
MUL color.rgb, color, tex.a;

MOV result.color.rgb, color;

END
