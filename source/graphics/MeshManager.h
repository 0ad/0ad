#ifndef __H_MESHMANAGER_H__
#define __H_MESHMANAGER_H__

#include "singleton.h"
#include "graphics/ModelDef.h"

#define g_MeshManager CMeshManager::GetSingleton()

typedef STL_HASH_MAP<size_t, CModelDef *> mesh_map;

class CMeshManager : public Singleton<CMeshManager>
{
public:
    CMeshManager();
    ~CMeshManager();

    CModelDef *GetMesh(const char *filename);
    int ReleaseMesh(CModelDef* mesh);
private:
    mesh_map m_MeshMap;
};

#endif