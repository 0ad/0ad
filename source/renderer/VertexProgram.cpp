#include "precompiled.h"
#include "res/vfs.h"
#include "res/mem.h"
#include "renderer/VertexProgram.h"
#include "graphics/ProgramManager.h"
#include "renderer/Renderer.h"
#include "maths/Vector3D.h"
#include "graphics/LightEnv.h"
#include "CLogger.h"

#define LOG_CATEGORY "shaders"

CVertexProgram::CVertexProgram(const char *file)
{
    Load(file);
}

CVertexProgram::~CVertexProgram()
{
}

bool CVertexProgram::IsValid()
{
    return false;
}

void CVertexProgram::Bind()
{
}

void CVertexProgram::Load(const char *file)
{
#ifdef BUILD_CG
/*    m_Program = NULL;
    if(!file || !vfs_exists(file) || g_ProgramManager.GetVPProfile() == CG_PROFILE_UNKNOWN)
        return;

    void *data;
    size_t size;
    Handle h = vfs_load(file, data, size, 0);
    if(h <= 0)
    {
        LOG(ERROR, LOG_CATEGORY, "CVertexShader::LoadShader: vfs_load for %s failed!\n", file);
        return;
    }

    std::string src = (char*)data;
    mem_free_h(h);

    m_Program = cgCreateProgram(
        g_ProgramManager.GetContext(),
        CG_SOURCE,
        src.c_str(),
        g_ProgramManager.GetVPProfile(),
        "main",
        NULL
    );
    if(!m_Program)
    {
        LOG(ERROR, LOG_CATEGORY, "CVertexShader::LoadShader: Could not compile Cg shader: %s", cgGetErrorString(cgGetError()));
        return;
    }*/
#endif
}
