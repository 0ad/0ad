#ifndef _OBJECTMANAGER_H
#define _OBJECTMANAGER_H

#include <vector>
#include <map>
#include "Singleton.h"
#include "CStr.h"
#include "ObjectBase.h"

class CObjectEntry;
class CBaseEntity;
class CMatrix3D;

// access to sole CObjectManager object
#define g_ObjMan CObjectManager::GetSingleton()

// Slight hack, to allow ScEd to place either entities or objects
class CObjectThing
{
public:
	virtual ~CObjectThing() {}
	virtual void Create(CMatrix3D& transform, int playerID)=0;
	virtual void SetTransform(CMatrix3D& transform)=0;
	virtual CObjectEntry* GetObjectEntry()=0;
};

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

	struct SObjectType
	{
		// name of this object type (derived from directory name)
		CStr m_Name;
		// index in parent array
		int m_Index;
		// list of objects of this type (found from the objects directory)
		std::map<ObjectKey, CObjectEntry*> m_Objects;
		std::map<CStr, CObjectBase*> m_ObjectBases;
	};

public:
	CObjectThing* m_SelectedThing;

	// constructor, destructor
	CObjectManager();
	~CObjectManager();

	int LoadObjects();
	void UnloadObjects();

	void AddObjectType(const char* name);

	CObjectEntry* FindObject(const char* objname);
	void AddObject(ObjectKey& key, CObjectEntry* entry, int type);
	void DeleteObject(CObjectEntry* entry);
	
	CObjectBase* FindObjectBase(const char* objname);

	CObjectEntry* FindObjectVariation(const char* objname, const std::vector<std::set<CStrW> >& selections);
	CObjectEntry* FindObjectVariation(CObjectBase* base, const std::vector<std::set<CStrW> >& selections);

	// Get all names, quite slowly. (Intended only for ScEd.)
	void GetAllObjectNames(std::vector<CStr>& names);
	void GetPropObjectNames(std::vector<CStr>& names);

	std::vector<SObjectType> m_ObjectTypes;
};


// Global comparison operator
bool operator< (const CObjectManager::ObjectKey& a, const CObjectManager::ObjectKey& b);

#endif
