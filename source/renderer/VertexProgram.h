#ifndef __H_VERTEXPROGRAM_H__
#define __H_VERTEXPROGRAM_H__

#include "ogl.h"

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
};

#endif
