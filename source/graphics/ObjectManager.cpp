#include "precompiled.h"

#include "ObjectManager.h"
#include <algorithm>
#include "CLogger.h"
#include "lib/res/res.h"
#include "timer.h"

#define LOG_CATEGORY "graphics"

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

CObjectEntry* CObjectManager::FindObjectByFileName(const char* filename)
{
	for (uint k=0;k<m_ObjectTypes.size();k++) {
		std::vector<CObjectEntry*>& objects=m_ObjectTypes[k].m_Objects;

		for (uint i=0;i<objects.size();i++) {
			if (strcmp(filename,(const char*) objects[i]->m_FileName)==0) {
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
	type.m_Index=(int)m_ObjectTypes.size()-1;
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
	TIMER(__CObjectManager__LoadObjects);

	// find all the object types by directory name
	BuildObjectTypes(0);

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

void CObjectManager::BuildObjectTypes(const char* base)
{
	CStr rootdir("art/actors/");
	if (base) {
		rootdir+=base;		
		rootdir+='/';
	}

	Handle dir=vfs_open_dir(rootdir);
	vfsDirEnt dent;
	if (dir > 0) {
		// Iterate subdirs
		while (vfs_next_dirent(dir, &dent, "/")==0)
		{
			CStr groupname;
			if (base) {
				groupname=base;
				groupname+='/';
				groupname+=dent.name;
			} else {
				groupname=dent.name;
		}
			AddObjectType(groupname);
			BuildObjectTypes(groupname);
	}
		vfs_close_dir(dir);
	} else {
		LOG(ERROR, LOG_CATEGORY, "CObjectManager::BuildObjectTypes(): Unable to open dir art/actors/");
	}
}

void CObjectManager::LoadObjects(int type)
{
	CStr pathname("art/actors/");
	pathname += m_ObjectTypes[type].m_Name;
	pathname += "/";

	Handle dir=vfs_open_dir(pathname);
	vfsDirEnt dent;
	
	if (dir > 0)
	{
		while (vfs_next_dirent(dir, &dent, "*.xml")==0)
		{
			CObjectEntry* object=new CObjectEntry(type);
			CStr filename(pathname);
			filename+=dent.name;
			if (!object->Load((const char*) filename)) {
				LOG(ERROR, LOG_CATEGORY, "CObjectManager::LoadObjects(): %s: XML Load failed", filename.c_str());
				delete object;
			} else {
				object->m_FileName=dent.name;
				AddObject(object,type);
				//LOG(NORMAL, LOG_CATEGORY, "CObjectManager::LoadObjects(): %s: XML Loaded", filename.c_str());
			}
		}
		vfs_close_dir(dir);
	}
	else
		LOG(ERROR, LOG_CATEGORY, "CObjectManager::LoadObjects(): Unable to open dir %s.", pathname.c_str());
}
