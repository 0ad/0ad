#ifndef __H_SHADERMANAGER_H__
#define __H_SHADERMANAGER_H__

#if _MSC_VER < 1300
#   include <map>
#   define hash_map map
#else
#   include <hash_map>
#endif

#include <string>
#include "Singleton.h"
#include "renderer/VertexProgram.h"

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
    std::hash_map<std::string, CVertexProgram*> m_VertexProgs;
#ifdef BUILD_CG
    CGcontext m_Context;
    CGprofile m_VPProfile;
    CGprofile m_FPProfile;
#endif
};

#endif