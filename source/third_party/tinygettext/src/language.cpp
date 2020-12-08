// tinygettext - A gettext replacement that works directly on .po files
// Copyright (c) 2006 Ingo Ruhnke <grumbel@gmail.com>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgement in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "precompiled.h"

#include "tinygettext/language.hpp"

#include <assert.h>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace tinygettext {

struct LanguageSpec {
  /** Language code: "de", "en", ... */
  const char* language;

  /** Country code: "BR", "DE", ..., can be 0 */
  const char* country;

  /** Modifier/Varint: "Latn", "ije", "latin"..., can be 0 */
  const char* modifier;

  /** Language name: "German", "English", "French", ... */
  const char* name;
};

/** Language Definitions */
//*{
static const LanguageSpec languages[] = {
  { "aa", nullptr,    nullptr, "Afar"                        },
  { "af", nullptr,    nullptr, "Afrikaans"                   },
  { "af", "ZA", nullptr, "Afrikaans (South Africa)"    },
  { "am", nullptr,    nullptr, "Amharic"                     },
  { "ar", nullptr,    nullptr, "Arabic"                      },
  { "ar", "AR", nullptr, "Arabic (Argentina)"          },
  { "ar", "OM", nullptr, "Arabic (Oman)"               },
  { "ar", "SA", nullptr, "Arabic (Saudi Arabia)"       },
  { "ar", "SY", nullptr, "Arabic (Syrian Arab Republic)" },
  { "ar", "TN", nullptr, "Arabic (Tunisia)"            },
  { "as", nullptr,    nullptr, "Assamese"                    },
  { "ast",nullptr,    nullptr, "Asturian"                    },
  { "ay", nullptr,    nullptr, "Aymara"                      },
  { "az", nullptr,    nullptr, "Azerbaijani"                 },
  { "az", "IR", nullptr, "Azerbaijani (Iran)"          },
  { "be", nullptr,    nullptr, "Belarusian"                  },
  { "be", nullptr, "latin", "Belarusian"               },
  { "bg", nullptr,    nullptr, "Bulgarian"                   },
  { "bg", "BG", nullptr, "Bulgarian (Bulgaria)"        },
  { "bn", nullptr,    nullptr, "Bengali"                     },
  { "bn", "BD", nullptr, "Bengali (Bangladesh)"        },
  { "bn", "IN", nullptr, "Bengali (India)"             },
  { "bo", nullptr,    nullptr, "Tibetan"                     },
  { "br", nullptr,    nullptr, "Breton"                      },
  { "bs", nullptr,    nullptr, "Bosnian"                     },
  { "bs", "BA", nullptr, "Bosnian (Bosnia/Herzegovina)"},
  { "bs", "BS", nullptr, "Bosnian (Bahamas)"           },
  { "ca", "ES", "valencia", "Catalan (valencia)" },
  { "ca", "ES", nullptr, "Catalan (Spain)"             },
  { "ca", nullptr,    "valencia", "Catalan (valencia)" },
  { "ca", nullptr,    nullptr, "Catalan"                     },
  { "cmn", nullptr,    nullptr, "Mandarin"                   },
  { "co", nullptr,    nullptr, "Corsican"                    },
  { "cs", nullptr,    nullptr, "Czech"                       },
  { "cs", "CZ", nullptr, "Czech (Czech Republic)"      },
  { "cy", nullptr,    nullptr, "Welsh"                       },
  { "cy", "GB", nullptr, "Welsh (Great Britain)"       },
  { "cz", nullptr,    nullptr, "Unknown language"            },
  { "da", nullptr,    nullptr, "Danish"                      },
  { "da", "DK", nullptr, "Danish (Denmark)"            },
  { "de", nullptr,    nullptr, "German"                      },
  { "de", "AT", nullptr, "German (Austria)"            },
  { "de", "CH", nullptr, "German (Switzerland)"        },
  { "de", "DE", nullptr, "German (Germany)"            },
  { "dk", nullptr,    nullptr, "Unknown language"            },
  { "dz", nullptr,    nullptr, "Dzongkha"                    },
  { "el", nullptr,    nullptr, "Greek"                       },
  { "el", "GR", nullptr, "Greek (Greece)"              },
  { "en", nullptr,    nullptr, "English"                     },
  { "en", "AU", nullptr, "English (Australia)"         },
  { "en", "CA", nullptr, "English (Canada)"            },
  { "en", "GB", nullptr, "English (Great Britain)"     },
  { "en", "US", nullptr, "English (United States)"     },
  { "en", "ZA", nullptr, "English (South Africa)"      },
  { "en", nullptr, "boldquot", "English"               },
  { "en", nullptr, "quot", "English"                   },
  { "en", "US", "piglatin", "English"            },
  { "eo", nullptr,    nullptr, "Esperanto"                   },
  { "es", nullptr,    nullptr, "Spanish"                     },
  { "es", "AR", nullptr, "Spanish (Argentina)"         },
  { "es", "CL", nullptr, "Spanish (Chile)"             },
  { "es", "CO", nullptr, "Spanish (Colombia)"          },
  { "es", "CR", nullptr, "Spanish (Costa Rica)"        },
  { "es", "DO", nullptr, "Spanish (Dominican Republic)"},
  { "es", "EC", nullptr, "Spanish (Ecuador)"           },
  { "es", "ES", nullptr, "Spanish (Spain)"             },
  { "es", "GT", nullptr, "Spanish (Guatemala)"         },
  { "es", "HN", nullptr, "Spanish (Honduras)"          },
  { "es", "LA", nullptr, "Spanish (Laos)"              },
  { "es", "MX", nullptr, "Spanish (Mexico)"            },
  { "es", "NI", nullptr, "Spanish (Nicaragua)"         },
  { "es", "PA", nullptr, "Spanish (Panama)"            },
  { "es", "PE", nullptr, "Spanish (Peru)"              },
  { "es", "PR", nullptr, "Spanish (Puerto Rico)"       },
  { "es", "SV", nullptr, "Spanish (El Salvador)"       },
  { "es", "UY", nullptr, "Spanish (Uruguay)"           },
  { "es", "VE", nullptr, "Spanish (Venezuela)"         },
  { "et", nullptr,    nullptr, "Estonian"                    },
  { "et", "EE", nullptr, "Estonian (Estonia)"          },
  { "et", "ET", nullptr, "Estonian (Ethiopia)"         },
  { "eu", nullptr,    nullptr, "Basque"                      },
  { "eu", "ES", nullptr, "Basque (Spain)"              },
  { "fa", nullptr,    nullptr, "Persian"                     },
  { "fa", "AF", nullptr, "Persian (Afghanistan)"       },
  { "fa", "IR", nullptr, "Persian (Iran)"              },
  { "fi", nullptr,    nullptr, "Finnish"                     },
  { "fi", "FI", nullptr, "Finnish (Finland)"           },
  { "fo", nullptr,    nullptr, "Faroese"                     },
  { "fo", "FO", nullptr, "Faeroese (Faroe Islands)"    },
  { "fr", nullptr,    nullptr, "French"                      },
  { "fr", "CA", nullptr, "French (Canada)"             },
  { "fr", "CH", nullptr, "French (Switzerland)"        },
  { "fr", "FR", nullptr, "French (France)"             },
  { "fr", "LU", nullptr, "French (Luxembourg)"         },
  { "fy", nullptr,    nullptr, "Frisian"                     },
  { "ga", nullptr,    nullptr, "Irish"                       },
  { "gd", nullptr,    nullptr, "Gaelic Scots"                },
  { "gl", nullptr,    nullptr, "Galician"                    },
  { "gl", "ES", nullptr, "Galician (Spain)"            },
  { "gn", nullptr,    nullptr, "Guarani"                     },
  { "gu", nullptr,    nullptr, "Gujarati"                    },
  { "gv", nullptr,    nullptr, "Manx"                        },
  { "ha", nullptr,    nullptr, "Hausa"                       },
  { "he", nullptr,    nullptr, "Hebrew"                      },
  { "he", "IL", nullptr, "Hebrew (Israel)"             },
  { "hi", nullptr,    nullptr, "Hindi"                       },
  { "hr", nullptr,    nullptr, "Croatian"                    },
  { "hr", "HR", nullptr, "Croatian (Croatia)"          },
  { "hu", nullptr,    nullptr, "Hungarian"                   },
  { "hu", "HU", nullptr, "Hungarian (Hungary)"         },
  { "hy", nullptr,    nullptr, "Armenian"                    },
  { "ia", nullptr,    nullptr, "Interlingua"                 },
  { "id", nullptr,    nullptr, "Indonesian"                  },
  { "id", "ID", nullptr, "Indonesian (Indonesia)"      },
  { "is", nullptr,    nullptr, "Icelandic"                   },
  { "is", "IS", nullptr, "Icelandic (Iceland)"         },
  { "it", nullptr,    nullptr, "Italian"                     },
  { "it", "CH", nullptr, "Italian (Switzerland)"       },
  { "it", "IT", nullptr, "Italian (Italy)"             },
  { "iu", nullptr,    nullptr, "Inuktitut"                   },
  { "ja", nullptr,    nullptr, "Japanese"                    },
  { "ja", "JP", nullptr, "Japanese (Japan)"            },
  { "ka", nullptr,    nullptr, "Georgian"                    },
  { "kk", nullptr,    nullptr, "Kazakh"                      },
  { "kl", nullptr,    nullptr, "Kalaallisut"                 },
  { "km", nullptr,    nullptr, "Khmer"                       },
  { "km", "KH", nullptr, "Khmer (Cambodia)"            },
  { "kn", nullptr,    nullptr, "Kannada"                     },
  { "ko", nullptr,    nullptr, "Korean"                      },
  { "ko", "KR", nullptr, "Korean (Korea)"              },
  { "ku", nullptr,    nullptr, "Kurdish"                     },
  { "kw", nullptr,    nullptr, "Cornish"                     },
  { "ky", nullptr,    nullptr, "Kirghiz"                     },
  { "la", nullptr,    nullptr, "Latin"                       },
  { "lo", nullptr,    nullptr, "Lao"                         },
  { "lt", nullptr,    nullptr, "Lithuanian"                  },
  { "lt", "LT", nullptr, "Lithuanian (Lithuania)"      },
  { "lv", nullptr,    nullptr, "Latvian"                     },
  { "lv", "LV", nullptr, "Latvian (Latvia)"            },
  { "jbo", nullptr,    nullptr, "Lojban"                     },
  { "mg", nullptr,    nullptr, "Malagasy"                    },
  { "mi", nullptr,    nullptr, "Maori"                       },
  { "mk", nullptr,    nullptr, "Macedonian"                  },
  { "mk", "MK", nullptr, "Macedonian (Macedonia)"      },
  { "ml", nullptr,    nullptr, "Malayalam"                   },
  { "mn", nullptr,    nullptr, "Mongolian"                   },
  { "mr", nullptr,    nullptr, "Marathi"                     },
  { "ms", nullptr,    nullptr, "Malay"                       },
  { "ms", "MY", nullptr, "Malay (Malaysia)"            },
  { "mt", nullptr,    nullptr, "Maltese"                     },
  { "my", nullptr,    nullptr, "Burmese"                     },
  { "my", "MM", nullptr, "Burmese (Myanmar)"           },
  { "nb", nullptr,    nullptr, "Norwegian Bokmal"            },
  { "nb", "NO", nullptr, "Norwegian Bokmål (Norway)"   },
  { "ne", nullptr,    nullptr, "Nepali"                      },
  { "nl", nullptr,    nullptr, "Dutch"                       },
  { "nl", "BE", nullptr, "Dutch (Belgium)"             },
  { "nl", "NL", nullptr, "Dutch (Netherlands)"         },
  { "nn", nullptr,    nullptr, "Norwegian Nynorsk"           },
  { "nn", "NO", nullptr, "Norwegian Nynorsk (Norway)"  },
  { "no", nullptr,    nullptr, "Norwegian"                   },
  { "no", "NO", nullptr, "Norwegian (Norway)"          },
  { "no", "NY", nullptr, "Norwegian (NY)"              },
  { "nr", nullptr,    nullptr, "Ndebele, South"              },
  { "oc", nullptr,    nullptr, "Occitan post 15nullptrnullptr"           },
  { "om", nullptr,    nullptr, "Oromo"                       },
  { "or", nullptr,    nullptr, "Oriya"                       },
  { "pa", nullptr,    nullptr, "Punjabi"                     },
  { "pl", nullptr,    nullptr, "Polish"                      },
  { "pl", "PL", nullptr, "Polish (Poland)"             },
  { "ps", nullptr,    nullptr, "Pashto"                      },
  { "pt", nullptr,    nullptr, "Portuguese"                  },
  { "pt", "BR", nullptr, "Portuguese (Brazil)"         },
  { "pt", "PT", nullptr, "Portuguese (Portugal)"       },
  { "qu", nullptr,    nullptr, "Quechua"                     },
  { "rm", nullptr,    nullptr, "Rhaeto-Romance"              },
  { "ro", nullptr,    nullptr, "Romanian"                    },
  { "ro", "RO", nullptr, "Romanian (Romania)"          },
  { "ru", nullptr,    nullptr, "Russian"                     },
  { "ru", "RU", nullptr, "Russian (Russia"             },
  { "rw", nullptr,    nullptr, "Kinyarwanda"                 },
  { "sa", nullptr,    nullptr, "Sanskrit"                    },
  { "sd", nullptr,    nullptr, "Sindhi"                      },
  { "se", nullptr,    nullptr, "Sami"                        },
  { "se", "NO", nullptr, "Sami (Norway)"               },
  { "si", nullptr,    nullptr, "Sinhalese"                   },
  { "sk", nullptr,    nullptr, "Slovak"                      },
  { "sk", "SK", nullptr, "Slovak (Slovakia)"           },
  { "sl", nullptr,    nullptr, "Slovenian"                   },
  { "sl", "SI", nullptr, "Slovenian (Slovenia)"        },
  { "sl", "SL", nullptr, "Slovenian (Sierra Leone)"    },
  { "sm", nullptr,    nullptr, "Samoan"                      },
  { "so", nullptr,    nullptr, "Somali"                      },
  { "sp", nullptr,    nullptr, "Unknown language"            },
  { "sq", nullptr,    nullptr, "Albanian"                    },
  { "sq", "AL", nullptr, "Albanian (Albania)"          },
  { "sr", nullptr,    nullptr, "Serbian"                     },
  { "sr", "YU", nullptr, "Serbian (Yugoslavia)"        },
  { "sr", nullptr,"ije", "Serbian"                     },
  { "sr", nullptr, "latin", "Serbian"                  },
  { "sr", nullptr, "Latn",  "Serbian"                  },
  { "ss", nullptr,    nullptr, "Swati"                       },
  { "st", nullptr,    nullptr, "Sotho"                       },
  { "sv", nullptr,    nullptr, "Swedish"                     },
  { "sv", "SE", nullptr, "Swedish (Sweden)"            },
  { "sv", "SV", nullptr, "Swedish (El Salvador)"       },
  { "sw", nullptr,    nullptr, "Swahili"                     },
  { "ta", nullptr,    nullptr, "Tamil"                       },
  { "te", nullptr,    nullptr, "Telugu"                      },
  { "tg", nullptr,    nullptr, "Tajik"                       },
  { "th", nullptr,    nullptr, "Thai"                        },
  { "th", "TH", nullptr, "Thai (Thailand)"             },
  { "ti", nullptr,    nullptr, "Tigrinya"                    },
  { "tk", nullptr,    nullptr, "Turkmen"                     },
  { "tl", nullptr,    nullptr, "Tagalog"                     },
  { "to", nullptr,    nullptr, "Tonga"                       },
  { "tr", nullptr,    nullptr, "Turkish"                     },
  { "tr", "TR", nullptr, "Turkish (Turkey)"            },
  { "ts", nullptr,    nullptr, "Tsonga"                      },
  { "tt", nullptr,    nullptr, "Tatar"                       },
  { "ug", nullptr,    nullptr, "Uighur"                      },
  { "uk", nullptr,    nullptr, "Ukrainian"                   },
  { "uk", "UA", nullptr, "Ukrainian (Ukraine)"         },
  { "ur", nullptr,    nullptr, "Urdu"                        },
  { "ur", "PK", nullptr, "Urdu (Pakistan)"             },
  { "uz", nullptr,    nullptr, "Uzbek"                       },
  { "uz", nullptr, "cyrillic", "Uzbek"                 },
  { "vi", nullptr,    nullptr, "Vietnamese"                  },
  { "vi", "VN", nullptr, "Vietnamese (Vietnam)"        },
  { "wa", nullptr,    nullptr, "Walloon"                     },
  { "wo", nullptr,    nullptr, "Wolof"                       },
  { "xh", nullptr,    nullptr, "Xhosa"                       },
  { "yi", nullptr,    nullptr, "Yiddish"                     },
  { "yo", nullptr,    nullptr, "Yoruba"                      },
  { "zh", nullptr,    nullptr, "Chinese"                     },
  { "zh", "CN", nullptr, "Chinese (simplified)"        },
  { "zh", "HK", nullptr, "Chinese (Hong Kong)"         },
  { "zh", "TW", nullptr, "Chinese (traditional)"       },
  { "zu", nullptr,    nullptr, "Zulu"                        },
  { nullptr, nullptr,    nullptr, nullptr                          }
};
//*}

namespace {

std::string
resolve_language_alias(const std::string& name)
{
  typedef std::unordered_map<std::string, std::string> Aliases;
  static Aliases language_aliases;
  if (language_aliases.empty())
  {
    // FIXME: Many of those are not useful for us, since we leave
    // encoding to the app, not to the language, we could/should
    // also match against all language names, not just aliases from
    // locale.alias

    // Aliases taken from /etc/locale.alias
    language_aliases["bokmal"]           = "nb_NO.ISO-8859-1";
    language_aliases["bokmål"]           = "nb_NO.ISO-8859-1";
    language_aliases["catalan"]          = "ca_ES.ISO-8859-1";
    language_aliases["croatian"]         = "hr_HR.ISO-8859-2";
    language_aliases["czech"]            = "cs_CZ.ISO-8859-2";
    language_aliases["danish"]           = "da_DK.ISO-8859-1";
    language_aliases["dansk"]            = "da_DK.ISO-8859-1";
    language_aliases["deutsch"]          = "de_DE.ISO-8859-1";
    language_aliases["dutch"]            = "nl_NL.ISO-8859-1";
    language_aliases["eesti"]            = "et_EE.ISO-8859-1";
    language_aliases["estonian"]         = "et_EE.ISO-8859-1";
    language_aliases["finnish"]          = "fi_FI.ISO-8859-1";
    language_aliases["français"]         = "fr_FR.ISO-8859-1";
    language_aliases["french"]           = "fr_FR.ISO-8859-1";
    language_aliases["galego"]           = "gl_ES.ISO-8859-1";
    language_aliases["galician"]         = "gl_ES.ISO-8859-1";
    language_aliases["german"]           = "de_DE.ISO-8859-1";
    language_aliases["greek"]            = "el_GR.ISO-8859-7";
    language_aliases["hebrew"]           = "he_IL.ISO-8859-8";
    language_aliases["hrvatski"]         = "hr_HR.ISO-8859-2";
    language_aliases["hungarian"]        = "hu_HU.ISO-8859-2";
    language_aliases["icelandic"]        = "is_IS.ISO-8859-1";
    language_aliases["italian"]          = "it_IT.ISO-8859-1";
    language_aliases["japanese"]         = "ja_JP.eucJP";
    language_aliases["japanese.euc"]     = "ja_JP.eucJP";
    language_aliases["ja_JP"]            = "ja_JP.eucJP";
    language_aliases["ja_JP.ujis"]       = "ja_JP.eucJP";
    language_aliases["japanese.sjis"]    = "ja_JP.SJIS";
    language_aliases["korean"]           = "ko_KR.eucKR";
    language_aliases["korean.euc"]       = "ko_KR.eucKR";
    language_aliases["ko_KR"]            = "ko_KR.eucKR";
    language_aliases["lithuanian"]       = "lt_LT.ISO-8859-13";
    language_aliases["no_NO"]            = "nb_NO.ISO-8859-1";
    language_aliases["no_NO.ISO-8859-1"] = "nb_NO.ISO-8859-1";
    language_aliases["norwegian"]        = "nb_NO.ISO-8859-1";
    language_aliases["nynorsk"]          = "nn_NO.ISO-8859-1";
    language_aliases["polish"]           = "pl_PL.ISO-8859-2";
    language_aliases["portuguese"]       = "pt_PT.ISO-8859-1";
    language_aliases["romanian"]         = "ro_RO.ISO-8859-2";
    language_aliases["russian"]          = "ru_RU.ISO-8859-5";
    language_aliases["slovak"]           = "sk_SK.ISO-8859-2";
    language_aliases["slovene"]          = "sl_SI.ISO-8859-2";
    language_aliases["slovenian"]        = "sl_SI.ISO-8859-2";
    language_aliases["spanish"]          = "es_ES.ISO-8859-1";
    language_aliases["swedish"]          = "sv_SE.ISO-8859-1";
    language_aliases["thai"]             = "th_TH.TIS-620";
    language_aliases["turkish"]          = "tr_TR.ISO-8859-9";
  }

  std::string name_lowercase;
  name_lowercase.resize(name.size());
  for(std::string::size_type i = 0; i < name.size(); ++i)
    name_lowercase[i] = static_cast<char>(tolower(name[i]));

  Aliases::iterator i = language_aliases.find(name_lowercase);
  if (i != language_aliases.end())
  {
    return i->second;
  }
  else
  {
    return name;
  }
}

} // namespace

Language
Language::from_spec(const std::string& language, const std::string& country, const std::string& modifier)
{
  typedef std::unordered_map<std::string, std::vector<const LanguageSpec*> > LanguageSpecMap;
  static LanguageSpecMap language_map;

  if (language_map.empty())
  { // Init language_map
    for(int i = 0; languages[i].language != nullptr; ++i)
      language_map[languages[i].language].push_back(&languages[i]);
  }

  LanguageSpecMap::iterator i = language_map.find(language);
  if (i != language_map.end())
  {
    std::vector<const LanguageSpec*>& lst = i->second;

    LanguageSpec tmpspec;
    tmpspec.language = language.c_str();
    tmpspec.country  = country.c_str();
    tmpspec.modifier = modifier.c_str();
    Language tmplang(&tmpspec);

    const LanguageSpec* best_match = nullptr;
    int best_match_score = 0;
    for(std::vector<const LanguageSpec*>::iterator j = lst.begin(); j != lst.end(); ++j)
    { // Search for the language that best matches the given spec, value country more then modifier
      int score = Language::match(Language(*j), tmplang);

      if (score > best_match_score)
      {
        best_match = *j;
        best_match_score = score;
      }
    }
    assert(best_match);
    return Language(best_match);
  }
  else
  {
    return Language();
  }
}

Language
Language::from_name(const std::string& spec_str)
{
  return from_env(resolve_language_alias(spec_str));
}

Language
Language::from_env(const std::string& env)
{
  // Split LANGUAGE_COUNTRY.CODESET@MODIFIER into parts
  std::string::size_type ln = env.find('_');
  std::string::size_type dt = env.find('.');
  std::string::size_type at = env.find('@');

  std::string language;
  std::string country;
  std::string codeset;
  std::string modifier;

  //std::cout << ln << " " << dt << " " << at << std::endl;

  language = env.substr(0, std::min(std::min(ln, dt), at));

  if (ln != std::string::npos && ln+1 < env.size()) // _
  {
    country = env.substr(ln+1, (std::min(dt, at) == std::string::npos) ? std::string::npos : std::min(dt, at) - (ln+1));
  }

  if (dt != std::string::npos && dt+1 < env.size()) // .
  {
    codeset = env.substr(dt+1, (at == std::string::npos) ? std::string::npos : (at - (dt+1)));
  }

  if (at != std::string::npos && at+1 < env.size()) // @
  {
    modifier = env.substr(at+1);
  }

  return from_spec(language, country, modifier);
}

Language::Language(const LanguageSpec* language_spec_)
  : language_spec(language_spec_)
{
}

Language::Language()
  : language_spec()
{
}

int
Language::match(const Language& lhs, const Language& rhs)
{
  if (lhs.get_language() != rhs.get_language())
  {
    return 0;
  }
  else
  {
    static int match_tbl[3][3] = {
      // modifier match, wildchard, miss
      { 9, 8, 5 }, // country match
      { 7, 6, 3 }, // country wildcard
      { 4, 2, 1 }, // country miss
    };

    int c;
    if (lhs.get_country() == rhs.get_country())
      c = 0;
    else if (lhs.get_country().empty() || rhs.get_country().empty())
      c = 1;
    else
      c = 2;

    int m;
    if (lhs.get_modifier() == rhs.get_modifier())
      m = 0;
    else if (lhs.get_modifier().empty() || rhs.get_modifier().empty())
      m = 1;
    else
      m = 2;

    return match_tbl[c][m];
  }
}

std::string
Language::get_language() const
{
  if (language_spec)
    return language_spec->language;
  else
    return "";
}

std::string
Language::get_country()  const
{
  if (language_spec && language_spec->country)
    return language_spec->country;
  else
    return "";
}

std::string
Language::get_modifier() const
{
  if (language_spec && language_spec->modifier)
    return language_spec->modifier;
  else
    return "";
}

std::string
Language::get_name()  const
{
  if (language_spec)
    return language_spec->name;
  else
    return "";
}

std::string
Language::str() const
{
  if (language_spec)
  {
    std::string var;
    var += language_spec->language;
    if (language_spec->country)
    {
      var += "_";
      var += language_spec->country;
    }

    if (language_spec->modifier)
    {
      var += "@";
      var += language_spec->modifier;
    }
    return var;
  }
  else
  {
    return "";
  }
}

bool
Language::operator==(const Language& rhs) const
{
  return language_spec == rhs.language_spec;
}

bool
Language::operator!=(const Language& rhs) const
{
  return language_spec != rhs.language_spec;
}

} // namespace tinygettext

/* EOF */
