#ifndef INCLUDED_MESHMANAGER
#define INCLUDED_MESHMANAGER

#include "ps/CStr.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class CModelDef;
typedef shared_ptr<CModelDef> CModelDefPtr;

class CColladaManager;

class CMeshManager
{
	NONCOPYABLE(CMeshManager);
public:
	CMeshManager(CColladaManager& colladaManager);
	~CMeshManager();

	CModelDefPtr GetMesh(const CStr& filename);

private:
	typedef STL_HASH_MAP<CStr, boost::weak_ptr<CModelDef>, CStr_hash_compare> mesh_map;
	mesh_map m_MeshMap;
	CColladaManager& m_ColladaManager;
};

#endif
