#ifndef __H_MESHMANAGER_H__
#define __H_MESHMANAGER_H__

#include "Singleton.h"
#include "graphics/ModelDef.h"

#define g_MeshManager CMeshManager::GetSingleton()

typedef STL_HASH_MAP<CStr, CModelDef *, CStr_hash_compare> mesh_map;

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
