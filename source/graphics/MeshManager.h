#ifndef __H_MESHMANAGER_H__
#define __H_MESHMANAGER_H__

#include "Singleton.h"
#include "CStr.h"

#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"

#define g_MeshManager CMeshManager::GetSingleton()

class CModelDef;
typedef boost::shared_ptr<CModelDef> CModelDefPtr;
typedef STL_HASH_MAP<CStr, boost::weak_ptr<CModelDef>, CStr_hash_compare> mesh_map;

class CMeshManager : public Singleton<CMeshManager>
{
public:
    CMeshManager();
    ~CMeshManager();

    CModelDefPtr GetMesh(const char *filename);
private:
    mesh_map m_MeshMap;
};

#endif
