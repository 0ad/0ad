#include "precompiled.h"
#include "graphics/ProgramManager.h"

CProgramManager::CProgramManager()
{
#ifdef BUILD_CG
    m_Context = cgCreateContext();
    assert(m_Context);
    FindPPVersion();
#endif
}

CProgramManager::~CProgramManager()
{
#ifdef BUILD_CG
    if(m_Context)
        cgDestroyContext(m_Context);
#endif
}

CVertexProgram *CProgramManager::FindVertexProgram(const char *file)
{
    CVertexProgram *prog = NULL;
    pp_map::iterator iter;

    if((iter = m_VertexProgs.find(std::string(file))) != m_VertexProgs.end())
        return (CVertexProgram *)(*iter).second;
    else
    {
        prog = new CVertexProgram(file);
        if(prog && prog->IsValid())
            m_VertexProgs[std::string(file)] = prog;
        else
            prog = NULL;
    }

    return prog;
}

void CProgramManager::Bind(CVertexProgram *prog)
{
#ifdef BUILD_CG
    assert(prog);
    if(m_VPProfile == CG_PROFILE_UNKNOWN || !prog->IsValid())
        return;

    prog->Bind();
#endif
}

void CProgramManager::WritePPInfo(FILE *file)
{
#ifdef BUILD_CG
    std::string version = "";
    switch(m_VPProfile)
    {
    case CG_PROFILE_VP30:
        version = "VP 3.0";
        break;
    case CG_PROFILE_VP20:
        version = "VP 2.0";
        break;
    case CG_PROFILE_ARBVP1:
        version = "ARB VP1";
        break;
    default:
        version = "No VP support";
        break;
    }
    fprintf(file, "Vertex Programs: %s\n", version.c_str());
    
    switch(m_FPProfile)
    {
    case CG_PROFILE_FP30:
        version = "FP 3.0";
        break;
    case CG_PROFILE_FP20:
        version = "FP 2.0";
        break;
    case CG_PROFILE_ARBVP1:
        version = "ARB FP1";
        break;
    default:
        version = "No FP support";
        break;
    }
    fprintf(file, "Fragment Programs: %s\n", version.c_str());
#else
    fprintf(file, "VP/FP support not compiled!\n");
#endif
}

#ifdef BUILD_CG

void CProgramManager::FindPPVersion()
{
#if BUILD_CG
    if(cgGLIsProfileSupported(CG_PROFILE_VP30))
        m_VPProfile = CG_PROFILE_VP30;
    else if(cgGLIsProfileSupported(CG_PROFILE_FP20))
        m_VPProfile = CG_PROFILE_VP20;
    else if(cgGLIsProfileSupported(CG_PROFILE_ARBVP1))
        m_VPProfile = CG_PROFILE_ARBVP1;
    else
        m_VPProfile = CG_PROFILE_UNKNOWN;

    if(cgGLIsProfileSupported(CG_PROFILE_FP30))
        m_FPProfile = CG_PROFILE_FP30;
    else if(cgGLIsProfileSupported(CG_PROFILE_FP20))
        m_FPProfile = CG_PROFILE_FP20;
    else if(cgGLIsProfileSupported(CG_PROFILE_ARBFP1))
        m_FPProfile = CG_PROFILE_ARBFP1;
    else
        m_FPProfile = CG_PROFILE_UNKNOWN;
#endif
}

#endif
