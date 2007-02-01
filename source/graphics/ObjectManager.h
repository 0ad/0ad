#ifndef _OBJECTMANAGER_H
#define _OBJECTMANAGER_H

#include <vector>
#include <map>
#include "ps/CStr.h"
#include "ObjectBase.h"

class CObjectEntry;
class CEntityTemplate;
class CMatrix3D;
class CMeshManager;

///////////////////////////////////////////////////////////////////////////////////////////
// CObjectManager: manager class for all possible actor types
class CObjectManager : boost::noncopyable
{
public:
	struct ObjectKey
	{
		ObjectKey(const CStr& name, const std::vector<u8>& var)
			: ActorName(name), ActorVariation(var) {}

		CStr ActorName;
		std::vector<u8> ActorVariation;

	};

public:

	// constructor, destructor
	CObjectManager(CMeshManager& meshManager);
	~CObjectManager();

	CMeshManager& GetMeshManager() const { return m_MeshManager; }

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

	std::map<ObjectKey, CObjectEntry*> m_Objects;
	std::map<CStr, CObjectBase*> m_ObjectBases;
};


// Global comparison operator
bool operator< (const CObjectManager::ObjectKey& a, const CObjectManager::ObjectKey& b);

#endif
