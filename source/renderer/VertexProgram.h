#ifndef __H_VERTEXPROGRAM_H__
#define __H_VERTEXPROGRAM_H__

#ifdef BUILD_CG

#pragma comment(lib, "cg.lib")
#pragma comment(lib, "cgGL.lib")

#include "ogl.h"
#include "Cg/Cg.h"
#include "Cg/cgGL.h"

#endif

class CProgramManager;

class CVertexProgram
{
friend class CProgramManager;
public:
    CVertexProgram(const char *file);
    ~CVertexProgram();

    bool IsValid();
private:
    void Bind();
    void Load(const char *file);
#ifdef BUILD_CG
    void PushParameter(CGparameter param);
    CGprogram m_Program;
#endif
};

#endif