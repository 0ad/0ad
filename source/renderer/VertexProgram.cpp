#include "precompiled.h"
#include "res/vfs.h"
#include "res/mem.h"
#include "renderer/VertexProgram.h"
#include "graphics/ProgramManager.h"
#include "CLogger.h"

#define LOG_CATEGORY "shaders"

CVertexProgram::CVertexProgram(const char *file)
{
    Load(file);
}

CVertexProgram::~CVertexProgram()
{
#ifdef BUILD_CG
    if(m_Program)
        cgDestroyProgram(m_Program);
#endif
}

bool CVertexProgram::IsValid()
{
#ifdef BUILD_CG
    return (m_Program != NULL);
#else
    return false;
#endif
}

void CVertexProgram::Bind()
{
#ifdef BUILD_CG
    if(!IsValid())
        return;

    CGparameter param = cgGetFirstParameter(m_Program, CG_PROGRAM);
    while(param)
    {
        PushParameter(param);
        param = cgGetNextParameter(param);
    }

    if(!cgGLIsProgramLoaded(m_Program))
        cgGLLoadProgram(m_Program);

    cgGLBindProgram(m_Program);
#endif
}

void CVertexProgram::Load(const char *file)
{
#ifdef BUILD_CG
    m_Program = NULL;
    if(!file || !vfs_exists(file))
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
        CG_PROFILE_ARBVP1,
        "main",
        NULL
    );
    if(!m_Program)
    {
        LOG(ERROR, LOG_CATEGORY, "CVertexShader::LoadShader: Could not compile Cg shader: %s", cgGetErrorString(cgGetError()));
        return;
    }
#endif
}

#ifdef BUILD_CG

void CVertexProgram::PushParameter(CGparameter param)
{
    assert(param);
    
    std::string name = cgGetParameterName(param);
    if(name == "ModelViewProj")
        cgGLSetStateMatrixParameter(param, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
    else if(name == "ModelView")
        cgGLSetStateMatrixParameter(param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_IDENTITY);
    else if(name == "Projection")
        cgGLSetStateMatrixParameter(param, CG_GL_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
    else if(name == "Texture")
        cgGLSetStateMatrixParameter(param, CG_GL_TEXTURE_MATRIX, CG_GL_MATRIX_IDENTITY);
    else
        LOG(WARNING, LOG_CATEGORY, "CVertexShader::LoadShader: Unknown parameter name: %s", name.c_str());
}

#endif
