#ifndef __H_MESHMANAGER_H__
#define __H_MESHMANAGER_H__

#include <boost/shared_ptr.hpp>

class CModelDef;
typedef boost::shared_ptr<CModelDef> CModelDefPtr;

class CStr8;

class CMeshManagerImpl;

class CMeshManager
{
public:
	CMeshManager();
	~CMeshManager();

	CModelDefPtr GetMesh(const CStr8& filename);

private:
	CMeshManagerImpl* m;
};

#endif
