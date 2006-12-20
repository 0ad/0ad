#ifndef __H_MESHMANAGER_H__
#define __H_MESHMANAGER_H__

#include "ps/Singleton.h"

#include <boost/shared_ptr.hpp>

class CModelDef;
typedef boost::shared_ptr<CModelDef> CModelDefPtr;

class CStr8;

#define g_MeshManager CMeshManager::GetSingleton()

class CMeshManagerImpl;

class CMeshManager : public Singleton<CMeshManager>
{
public:
	CMeshManager();
	~CMeshManager();

	CModelDefPtr GetMesh(const CStr8& filename);

private:
	CMeshManagerImpl* m;
};

#endif
