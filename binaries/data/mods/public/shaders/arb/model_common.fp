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

#if USE_FP_SHADOW && USE_SHADOW_PCF
  PARAM shadowScale = program.local[3];
  TEMP offset, size, weight;
#endif

#if USE_SPECULAR
  ATTRIB v_normal = fragment.texcoord[3];
  ATTRIB v_half = fragment.texcoord[4];
  PARAM specularPower = program.local[4];
  PARAM specularColor = program.local[5];
  PARAM sunColor = program.local[6];
#endif

TEMP tex;
TEMP texdiffuse;
TEMP sundiffuse;
TEMP temp;
TEMP color;
TEMP shadow;

TEX tex, v_tex, texture[0], 2D;

// Alpha-test as early as possible
#ifdef REQUIRE_ALPHA_GEQUAL
  SUB temp.x, tex.a, REQUIRE_ALPHA_GEQUAL;
  KIL temp.x; // discard if < 0.0
#endif

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
  // specular = sunColor * specularColor * pow(max(0.0, dot(normalize(v_normal), v_half)), specularPower);
  TEMP specular;
  TEMP normal;
  MUL specular.rgb, specularColor, sunColor;
  DP3 normal.w, v_normal, v_normal;
  RSQ normal.w, normal.w;
  MUL normal.xyz, v_normal, normal.w;
  DP3_SAT temp.y, normal, v_half;
  // temp^p = (2^lg2(temp))^p = 2^(lg2(temp)*p)
  LG2 temp.y, temp.y;
  MUL temp.y, temp.y, specularPower.x;
  EX2 temp.y, temp.y;
  // TODO: why not just use POW here? (should test performance first)
  MUL specular.rgb, specular, temp.y;
#endif

// color = (texdiffuse * sundiffuse + specular) * get_shadow() + texdiffuse * ambient;
// (sundiffuse is 2*fragment.color due to clamp-avoidance in the vertex program)
#if USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
  TEMP shadowBias;
  TEMP biasedShdw;
  MOV shadowBias.x, 0.003;
  MOV biasedShdw, v_shadow;
  SUB biasedShdw.z, v_shadow.z, shadowBias.x;
  #if USE_FP_SHADOW
    #if USE_SHADOW_PCF
      SUB offset.xy, v_shadow, 0.5;
      FRC offset.xy, offset;
      ADD size.xy, offset, 1.0;
      SUB size.zw, 2.0, offset.xyxy;

      MAD offset.xy, -0.5, offset, v_shadow;
      MOV offset.z, biasedShdw.z;
      ADD weight, { 1.0, 1.0, -0.5, -0.5 }, offset.xyxy;
      MUL weight, weight, shadowScale.zwzw;

      MOV offset.xy, weight.zwww;
      TEX temp.x, offset, texture[1], SHADOW2D;
      MOV offset.x, weight.x;
      TEX temp.y, offset, texture[1], SHADOW2D;
      MOV offset.xy, weight.zyyy;
      TEX temp.z, offset, texture[1], SHADOW2D;
      MOV offset.x, weight.x;
      TEX temp.w, offset, texture[1], SHADOW2D;

      MUL size, size.zxzx, size.wwyy;
      DP4 shadow.x, temp, size;
      MUL shadow.x, shadow.x, 0.111111;
    #else
      TEX shadow.x, biasedShdw, texture[1], SHADOW2D;
    #endif
  #else
    TEX tex, v_shadow, texture[1], 2D;
    MOV_SAT temp.z, biasedShdw.z;
    SGE shadow.x, tex.x, temp.z;
  #endif

  MUL sundiffuse.rgb, fragment.color, 2.0;

  #if USE_SPECULAR
    MAD color.rgb, texdiffuse, sundiffuse, specular;
    MUL temp.rgb, texdiffuse, ambient;
    MAD color.rgb, color, shadow.x, temp;
  #else
    MAD temp.rgb, sundiffuse, shadow.x, ambient;
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

#if !IGNORE_LOS
  // Multiply everything by the LOS texture
  TEX tex.a, v_los, texture[2], 2D;
  MUL color.rgb, color, tex.a;
#endif

MUL result.color.rgb, color, shadingColor;

END
