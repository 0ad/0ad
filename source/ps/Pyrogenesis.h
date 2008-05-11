/*
Pyrogenesis.h

Standard declarations which are included in all projects.
*/

#ifndef INCLUDED_PYROGENESIS
#define INCLUDED_PYROGENESIS

typedef const char * PS_RESULT;

#define DEFINE_ERROR(x, y)  PS_RESULT x=y
#define DECLARE_ERROR(x)  extern PS_RESULT x

DECLARE_ERROR(PS_OK);
DECLARE_ERROR(PS_FAIL);



#define MICROLOG debug_wprintf_mem


// overrides ah_translate. registered in GameSetup.cpp
extern const wchar_t* psTranslate(const wchar_t* text);
extern void psTranslateFree(const wchar_t* text);
extern void psBundleLogs(FILE* f);
extern const char* psGetLogDir();

#endif
