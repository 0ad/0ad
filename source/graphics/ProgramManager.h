#ifndef __H_PROGRAMMANAGER_H__
#define __H_PROGRAMMANAGER_H__

#include <string>
#include "Singleton.h"
#include "renderer/VertexProgram.h"

typedef STL_HASH_MAP<std::string, CVertexProgram *> pp_map;

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
    CGprofile GetVPProfile() { return m_VPProfile; }
    CGprofile GetFPProfile() { return m_FPProfile; }
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