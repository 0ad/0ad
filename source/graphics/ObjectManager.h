#ifndef _OBJECTMANAGER_H
#define _OBJECTMANAGER_H

#include <vector>
#include <map>
#include "ps/Singleton.h"
#include "ps/CStr.h"
#include "ObjectBase.h"

class CObjectEntry;
class CEntityTemplate;
class CMatrix3D;

// access to sole CObjectManager object
#define g_ObjMan CObjectManager::GetSingleton()

///////////////////////////////////////////////////////////////////////////////////////////
// CObjectManager: manager class for all possible actor types
class CObjectManager : public Singleton<CObjectManager>
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
	CObjectManager();
	~CObjectManager();

	void UnloadObjects();

	CObjectEntry* FindObject(const char* objname);
	void DeleteObject(CObjectEntry* entry);
	
	CObjectBase* FindObjectBase(const char* objname);

	CObjectEntry* FindObjectVariation(const char* objname, const std::vector<std::set<CStr> >& selections);
	CObjectEntry* FindObjectVariation(CObjectBase* base, const std::vector<std::set<CStr> >& selections);

	// Get all names, quite slowly. (Intended only for ScEd.)
	void GetAllObjectNames(std::vector<CStr>& names);
	void GetPropObjectNames(std::vector<CStr>& names);

	std::map<ObjectKey, CObjectEntry*> m_Objects;
	std::map<CStr, CObjectBase*> m_ObjectBases;
};


// Global comparison operator
bool operator< (const CObjectManager::ObjectKey& a, const CObjectManager::ObjectKey& b);

#endif
