#ifndef _OBJECTMANAGER_H
#define _OBJECTMANAGER_H

#include <vector>
#include <map>
#include "Singleton.h"
#include "CStr.h"
#include "ObjectBase.h"

class CObjectBase;
class CObjectEntry;
class CBaseEntity;

// access to sole CObjectManager object
#define g_ObjMan CObjectManager::GetSingleton()

///////////////////////////////////////////////////////////////////////////////////////////
// CObjectManager: manager class for all possible actor types
class CObjectManager : public Singleton<CObjectManager>
{
public:
	struct ObjectKey
	{
		ObjectKey(CStr& name, CObjectBase::variation_key& var)
			: ActorName(name), ActorVariation(var) {}

		CStr ActorName;
		CObjectBase::variation_key ActorVariation;

	};

	struct SObjectType
	{
		// name of this object type (derived from directory name)
		CStr m_Name;
		// index in parent array
		int m_Index;
		// list of objects of this type (found from the objects directory)
		std::map<ObjectKey, CObjectEntry*> m_Objects;
		std::map<CStr, CObjectBase*> m_ObjectBases;

		std::map<CStr, CStr> m_ObjectNameToFilename;
	};

public:
	// constructor, destructor
	CObjectManager();
	~CObjectManager();

	void LoadObjects();

	void AddObjectType(const char* name);

	CObjectEntry* FindObject(const char* objname);
	void AddObject(ObjectKey& key, CObjectEntry* entry, int type);
	void DeleteObject(CObjectEntry* entry);
	
	CObjectBase* FindObjectBase(const char* objname);
	void AddObjectBase(CObjectBase* base);
	void DeleteObjectBase(CObjectBase* base);

	CBaseEntity* m_SelectedEntity;

	std::vector<SObjectType> m_ObjectTypes;

private:
	void LoadObjectsIn(CStr& pathname);
};


// Global comparison operator
bool operator< (const CObjectManager::ObjectKey& a, const CObjectManager::ObjectKey& b);

#endif
