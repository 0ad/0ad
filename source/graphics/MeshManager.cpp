#include "precompiled.h"
#include "graphics/MeshManager.h"
#include "CLogger.h"
#include "FileUnpacker.h" // to get access to its CError
#include "ModelDef.h"

CMeshManager::CMeshManager()
{
}

CMeshManager::~CMeshManager()
{
}

CModelDefPtr CMeshManager::GetMesh(const char *filename)
{
	CStr fn(filename);
	mesh_map::iterator iter = m_MeshMap.find(fn);
	if (iter != m_MeshMap.end())
	{
		try
		{
			CModelDefPtr model (iter->second);
			LOG(MESSAGE, "mesh", "Loading mesh '%s%' (cached)...", filename);
			return model;
		}
		// If the mesh has already been deleted, the weak_ptr -> shared_ptr
		// conversion will throw bad_weak_ptr (and we need to reload the mesh)
		catch (boost::bad_weak_ptr)
		{
		}
	}

	try
	{
		CModelDefPtr model (CModelDef::Load(filename));
		if (!model)
			return CModelDefPtr();

		LOG(MESSAGE, "mesh", "Loading mesh '%s'...", filename);
		m_MeshMap[fn] = model;
		return model;
	}
	catch (CFileUnpacker::CError)
	{
		LOG(ERROR, "mesh", "Could not load mesh '%s'!", filename);
		return CModelDefPtr();
	}
}
