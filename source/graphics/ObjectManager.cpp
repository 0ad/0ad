#include "ObjectManager.h"
#include <io.h>
#include <algorithm>


CObjectManager::CObjectManager() : m_SelectedObject(0)
{
	m_ObjectTypes.reserve(32);
}

CObjectManager::~CObjectManager()
{
	m_SelectedObject=0;
	for (size_t i=0;i<m_ObjectTypes.size();i++) {
		for (size_t j=0;j<m_ObjectTypes[i].m_Objects.size();j++) {
			delete m_ObjectTypes[i].m_Objects[j];
		}
	}
}

CObjectEntry* CObjectManager::FindObject(const char* objectname)
{
	for (uint k=0;k<m_ObjectTypes.size();k++) {
		std::vector<CObjectEntry*>& objects=m_ObjectTypes[k].m_Objects;

		for (uint i=0;i<objects.size();i++) {
			if (strcmp(objectname,(const char*) objects[i]->m_Name)==0) {
				return objects[i];
			}
		}
	}

	return 0;
}

void CObjectManager::AddObjectType(const char* name)
{
	m_ObjectTypes.resize(m_ObjectTypes.size()+1);
	SObjectType& type=m_ObjectTypes.back();
	type.m_Name=name;
	type.m_Index=m_ObjectTypes.size()-1;
}

void CObjectManager::AddObject(CObjectEntry* object,int type)
{
	assert((uint)type<m_ObjectTypes.size());
	m_ObjectTypes[type].m_Objects.push_back(object);
}

void CObjectManager::DeleteObject(CObjectEntry* entry)
{
	std::vector<CObjectEntry*>& objects=m_ObjectTypes[entry->m_Type].m_Objects;

	typedef std::vector<CObjectEntry*>::iterator Iter;
	Iter i=std::find(objects.begin(),objects.end(),entry);
	if (i!=objects.end()) {
		objects.erase(i);
	}
	delete entry;
}

void CObjectManager::LoadObjects()
{
	// find all the object types by directory name
	BuildObjectTypes();

	// now iterate through terrain types loading all textures of that type
	uint i;
	for (i=0;i<m_ObjectTypes.size();i++) {
		LoadObjects(i);
	}
	
	// now build all the models
	for (i=0;i<m_ObjectTypes.size();i++) {
		std::vector<CObjectEntry*>& objects=m_ObjectTypes[i].m_Objects;

		for (uint j=0;j<objects.size();j++) {
			// object might have already been built (eg if it's a prop); 
			// and only build if we haven't a model
			if (!objects[j]->m_Model) {
				if (!objects[j]->BuildModel()) {
					DeleteObject(objects[j]);
				}
			}
		}
	}
}

void CObjectManager::BuildObjectTypes()
{
	struct _finddata_t file;
	long handle;
	
	// Find first matching directory in terrain\textures
    if ((handle=_findfirst("mods\\official\\art\\actors\\*",&file))!=-1) {
		
		if ((file.attrib & _A_SUBDIR) && file.name[0]!='.') {
			AddObjectType(file.name);
		}

		// Find the rest of the matching files
        while( _findnext(handle,&file)==0) {
			if ((file.attrib & _A_SUBDIR) && file.name[0]!='.') {
				AddObjectType(file.name);
			}
		}

        _findclose(handle);
	}
}

void CObjectManager::LoadObjects(int type)
{
	struct _finddata_t file;
	long handle;

	// build pathname
	CStr pathname("mods\\official\\art\\actors\\");
	pathname+=m_ObjectTypes[type].m_Name;
	pathname+="\\";
	
	CStr findname(pathname);
	findname+="*.xml";

	// Find first matching file in directory for this terrain type
    if ((handle=_findfirst((const char*) findname,&file))!=-1) {
		
		CObjectEntry* object=new CObjectEntry(type);
		CStr filename(pathname);
		filename+=file.name;
		if (!object->Load((const char*) filename)) {
			delete object;
		} else {
			AddObject(object,type);
		}

		// Find the rest of the matching files
        while( _findnext(handle,&file)==0) {
			CObjectEntry* object=new CObjectEntry(type);
			CStr filename(pathname);
			filename+=file.name;
			if (!object->Load((const char*) filename)) {
				delete object;
			} else {
				AddObject(object,type);
			}
		}

        _findclose(handle);
	}
}
