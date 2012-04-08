!!ARBfp1.0
#if USE_FP_SHADOW
  OPTION ARB_fragment_program_shadow;
#endif

ATTRIB v_tex = fragment.texcoord[0];
ATTRIB v_shadow = fragment.texcoord[1];
ATTRIB v_los = fragment.texcoord[2];

#if USE_OBJECTCOLOR
  PARAM objectColor = program.local[0];
#else
#if USE_PLAYERCOLOR
  PARAM playerColor = program.local[0];
#endif
#endif

PARAM shadingColor = program.local[1];
PARAM ambient = program.local[2];

#if USE_SHADOW_PCF
  PARAM shadowOffsets1 = program.local[3];
  PARAM shadowOffsets2 = program.local[4];
  TEMP offset;
#endif

#if USE_SPECULAR
  ATTRIB v_normal = fragment.texcoord[3];
  ATTRIB v_half = fragment.texcoord[4];
  PARAM specularPower = program.local[5];
  PARAM specularColor = program.local[6];
#endif

TEMP tex;
TEMP texdiffuse;
TEMP sundiffuse;
TEMP temp;
TEMP color;
TEMP shadow;

TEX tex, v_tex, texture[0], 2D;
#if USE_TRANSPARENT
  MOV result.color.a, tex;
#endif

// Apply coloring based on texture alpha
#if USE_OBJECTCOLOR
  LRP temp.rgb, objectColor, 1.0, tex.a;
  MUL texdiffuse.rgb, tex, temp;
#else
#if USE_PLAYERCOLOR
  LRP temp.rgb, playerColor, 1.0, tex.a;
  MUL texdiffuse.rgb, tex, temp;
#else
  MOV texdiffuse.rgb, tex;
#endif
#endif

#if USE_SPECULAR
  // specular = specularColor * pow(max(0.0, dot(normalize(v_normal), v_half)), specularPower);
  TEMP specular;
  TEMP normal;
  DP3 normal.w, v_normal, v_normal;
  RSQ normal.w, normal.w;
  MUL normal.xyz, v_normal, normal.w;
  DP3_SAT temp.y, normal, v_half;
  // temp^p = (2^lg2(temp))^p = 2^(lg2(temp)*p)
  LG2 temp.y, temp.y;
  MUL temp.y, temp.y, specularPower.x;
  EX2 temp.y, temp.y;
  MUL specular.rgb, specularColor, temp.y;
#endif

// color = (texdiffuse * sundiffuse + specular) * get_shadow() + texdiffuse * ambient;
// (sundiffuse is 2*fragment.color due to clamp-avoidance in the vertex program)
#if USE_SHADOW
  #if USE_FP_SHADOW
    #if USE_SHADOW_PCF
      MOV offset.zw, v_shadow;
      ADD offset.xy, v_shadow, shadowOffsets1;
      TEX temp.x, offset, texture[1], SHADOW2D;
      ADD offset.xy, v_shadow, shadowOffsets1.zwzw;
      TEX temp.y, offset, texture[1], SHADOW2D;
      ADD offset.xy, v_shadow, shadowOffsets2;
      TEX temp.z, offset, texture[1], SHADOW2D;
      ADD offset.xy, v_shadow, shadowOffsets2.zwzw;
      TEX temp.w, offset, texture[1], SHADOW2D;
      DP4 shadow, temp, 0.25;
    #else
      TEX shadow, v_shadow, texture[1], SHADOW2D;
    #endif
  #else
    TEX tex, v_shadow, texture[1], 2D;
    MOV_SAT temp.z, v_shadow.z;
    SGE shadow, tex.x, temp.z;
  #endif

  MUL sundiffuse.rgb, fragment.color, 2.0;

  #if USE_SPECULAR
    MAD color.rgb, texdiffuse, sundiffuse, specular;
    MUL temp.rgb, texdiffuse, ambient;
    MAD color.rgb, color, shadow, temp;
  #else
    MAD temp.rgb, sundiffuse, shadow, ambient;
    MUL color.rgb, texdiffuse, temp;
  #endif
  
#else
  #if USE_SPECULAR
    MAD temp.rgb, fragment.color, 2.0, ambient;
    MAD color.rgb, texdiffuse, temp, specular;
  #else
    MAD temp.rgb, fragment.color, 2.0, ambient;
    MUL color.rgb, texdiffuse, temp;
  #endif
#endif

// Multiply everything by the LOS texture
TEX tex.a, v_los, texture[2], 2D;
MUL color.rgb, color, tex.a;

MUL result.color.rgb, color, shadingColor;

END
