#ifndef __H_PROGRAMMANAGER_H__
#define __H_PROGRAMMANAGER_H__

#include <string>
#include "Singleton.h"
#include "renderer/VertexProgram.h"

#if (_MSC_VER < 1300) || (_MSC_VER > 1310)
#   include <map>
typedef std::map<std::string, CVertexProgram *> pp_map;
#else
#   include <hash_map>
typedef std::hash_map<std::string, CVertexProgram *> pp_map;
#endif

#define g_ProgramManager CProgramManager::GetSingleton()


class CProgramManager : public Singleton<CProgramManager>
{
public:
    CProgramManager();
    ~CProgramManager();

    CVertexProgram *FindVertexProgram(const char *file);
    void Bind(CVertexProgram *prog);

    void WritePPInfo(FILE *file);
#ifdef BUILD_CG
    CGcontext GetContext() { return m_Context; }
#endif
private:
#ifdef BUILD_CG
    void FindPPVersion();
#endif
    pp_map m_VertexProgs;
#ifdef BUILD_CG
    CGcontext m_Context;
    CGprofile m_VPProfile;
    CGprofile m_FPProfile;
#endif
};

#endif