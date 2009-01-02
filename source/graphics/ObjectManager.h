#ifndef INCLUDED_OBJECTMANAGER
#define INCLUDED_OBJECTMANAGER

#include <vector>
#include <map>
#include <set>
#include "ps/CStr.h"

class CEntityTemplate;
class CMatrix3D;
class CMeshManager;
class CObjectBase;
class CObjectEntry;
class CSkeletonAnimManager;

///////////////////////////////////////////////////////////////////////////////////////////
// CObjectManager: manager class for all possible actor types
class CObjectManager : noncopyable
{
public:
	struct ObjectKey
	{
		ObjectKey(const CStr& name, const std::vector<u8>& var)
			: ActorName(name), ActorVariation(var) {}

		CStr ActorName;
		std::vector<u8> ActorVariation;

		bool operator< (const CObjectManager::ObjectKey& a) const;
	};

public:

	// constructor, destructor
	CObjectManager(CMeshManager& meshManager, CSkeletonAnimManager& skeletonAnimManager);
	~CObjectManager();

	// Provide access to the manager classes for meshes and animations - they're
	// needed when objects are being created and so this seems like a convenient
	// place to centralise access.
	CMeshManager& GetMeshManager() const { return m_MeshManager; }
	CSkeletonAnimManager& GetSkeletonAnimManager() const { return m_SkeletonAnimManager; }

	void UnloadObjects();

	CObjectEntry* FindObject(const char* objname);
	void DeleteObject(CObjectEntry* entry);
	
	CObjectBase* FindObjectBase(const char* objname);

	CObjectEntry* FindObjectVariation(const char* objname, const std::vector<std::set<CStr> >& selections);
	CObjectEntry* FindObjectVariation(CObjectBase* base, const std::vector<std::set<CStr> >& selections);

	// Get all names, quite slowly. (Intended only for Atlas.)
	static void GetAllObjectNames(std::vector<CStr>& names);
	static void GetPropObjectNames(std::vector<CStr>& names);

private:
	CMeshManager& m_MeshManager;
	CSkeletonAnimManager& m_SkeletonAnimManager;

	std::map<ObjectKey, CObjectEntry*> m_Objects;
	std::map<CStr, CObjectBase*> m_ObjectBases;
};

#endif
