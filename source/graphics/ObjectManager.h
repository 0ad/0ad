#ifndef _OBJECTMANAGER_H
#define _OBJECTMANAGER_H

#include <vector>
#include "Singleton.h"
#include "ObjectEntry.h"

// access to sole CObjectManager object
#define g_ObjMan CObjectManager::GetSingleton()

///////////////////////////////////////////////////////////////////////////////////////////
// CObjectManager: manager class for all possible actor types
class CObjectManager : public Singleton<CObjectManager>
{
public:
	struct SObjectType
	{
		// name of this object type (derived from directory name)
		CStr m_Name;
		// index in parent array
		int m_Index;
		// list of objects of this type (found from the objects directory)
		std::map<CStr, CObjectEntry*> m_Objects;

		std::map<CStr, CStr> m_ObjectNameToFilename;
	};

public:
	// constructor, destructor
	CObjectManager();
	~CObjectManager();

	void LoadObjects();

	void AddObjectType(const char* name);

	CObjectEntry* FindObject(const char* objname);
	void AddObject(CObjectEntry* entry,int type);
	void DeleteObject(CObjectEntry* entry);

	CObjectEntry* GetSelectedObject() const { return m_SelectedObject; }
	void SetSelectedObject(CObjectEntry* obj) { m_SelectedObject=obj; }

	std::vector<SObjectType> m_ObjectTypes;

private:
	void LoadObjectsIn(CStr& pathname);

	CObjectEntry* m_SelectedObject;
};


#endif
