#include "precompiled.h"
#include "graphics/MeshManager.h"
#include "CLogger.h"

CMeshManager::CMeshManager()
{
}

CMeshManager::~CMeshManager()
{
}

CModelDef *CMeshManager::GetMesh(const char *filename)
{
    mesh_map::iterator iter;
    CStr fn(filename);
    if((iter = m_MeshMap.find(fn.GetHashCode())) == m_MeshMap.end())
    {
        try
        {
            CModelDef *model = CModelDef::Load(filename);
            if(!model)
                return NULL;

            LOG(MESSAGE, "mesh", "Loading mesh '%s'...\n", filename);
            model->m_Hash = fn.GetHashCode();
            model->m_RefCount = 1;
            m_MeshMap[model->m_Hash] = model;
            return model;
        }
        catch(...)
        {
            LOG(ERROR, "mesh", "Could not load mesh '%s'\n!", filename);
            return NULL;
        }
    }
    else
    {
        LOG(MESSAGE, "mesh", "Loading mesh '%s%' (cached)...\n", filename);
        CModelDef *model = (CModelDef *)(*iter).second;
        model->m_RefCount++;
        return model;
    }
}

int CMeshManager::ReleaseMesh(CModelDef *mesh)
{
    if(!mesh)
        return 0;

    mesh_map::iterator iter = m_MeshMap.find(mesh->m_Hash);
    if(iter == m_MeshMap.end())
        return 0;

    mesh->m_RefCount--;
    if(mesh->m_RefCount <= 0)
    {
        m_MeshMap.erase(iter);
        delete mesh;
        mesh = NULL;
        return 0;
    }

    return mesh->m_RefCount;
}
