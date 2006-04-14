#include "precompiled.h"

#include "ObjectManager.h"
#include <algorithm>
#include "CLogger.h"
#include "lib/res/res.h"
#include "timer.h"
#include "VFSUtil.h"
#include "ObjectBase.h"
#include "ObjectEntry.h"
#include "Entity.h"
#include "EntityManager.h"
#include "BaseEntity.h"
#include "Game.h"
#include "Model.h"
#include "Unit.h"
#include "Matrix3D.h"
#include "ps/Profile.h"

#define LOG_CATEGORY "graphics"

bool operator< (const CObjectManager::ObjectKey& a, const CObjectManager::ObjectKey& b)
{
	if (a.ActorName < b.ActorName)
		return true;
	else if (a.ActorName > b.ActorName)
		return false;
	else
		return a.ActorVariation < b.ActorVariation;
}

CObjectManager::CObjectManager() : m_SelectedThing(NULL)
{
	m_ObjectTypes.reserve(32);
}

template<typename T, typename S> static void delete_pair_2nd(std::pair<T,S> v) { delete v.second; }

CObjectManager::~CObjectManager()
{
	UnloadObjects();
}


// TODO (PT): Work out what the object 'types' are for, since they're not
// doing anything obvious. (And if they're useless, remove them.)

CObjectBase* CObjectManager::FindObjectBase(const char* objectname)
{
	// See if the base type has been loaded yet

	for (uint k = 0; k < m_ObjectTypes.size(); k++)
	{
		std::map<CStr, CObjectBase*>::iterator it = m_ObjectTypes[k].m_ObjectBases.find(objectname);
		if (it != m_ObjectTypes[k].m_ObjectBases.end())
			return it->second;
	}

	// Not already loaded, so try to load it:

	for (uint k = 0; k < m_ObjectTypes.size(); k++)
	{
		CObjectBase* obj = new CObjectBase();

		if (obj->Load(objectname))
		{
			m_ObjectTypes[k].m_ObjectBases[objectname] = obj;
			return obj;
		}
		else
			delete obj;
	}

	LOG(ERROR, LOG_CATEGORY, "CObjectManager::FindObjectBase(): Cannot find object '%s'", objectname);

	return 0;
}

CObjectEntry* CObjectManager::FindObject(const char* objname)
{
	std::vector<std::set<CStrW> > selections; // TODO - should this really be empty?
	return FindObjectVariation(objname, selections);
}

CObjectEntry* CObjectManager::FindObjectVariation(const char* objname, const std::vector<std::set<CStrW> >& selections)
{
	CObjectBase* base = FindObjectBase(objname);

	if (! base)
		return NULL;

	return FindObjectVariation(base, selections);
}

CObjectEntry* CObjectManager::FindObjectVariation(CObjectBase* base, const std::vector<std::set<CStrW> >& selections)
{
	PROFILE( "object variation loading" );

	// Look to see whether this particular variation has already been loaded

	std::vector<u8> choices = base->CalculateVariationKey(selections);
	ObjectKey key (base->m_Name, choices);

	std::map<ObjectKey, CObjectEntry*>::iterator it = m_ObjectTypes[0].m_Objects.find(key);
	if (it != m_ObjectTypes[0].m_Objects.end())
		return it->second;

	// If it hasn't been loaded, load it now

	CObjectEntry* obj = new CObjectEntry(0, base); // TODO: type ?

	// TODO (for some efficiency): use the pre-calculated choices for this object,
	// which has already worked out what to do for props, instead of passing the
	// selections into BuildVariation and having it recalculate the props' choices.

	if (! obj->BuildVariation(selections, choices))
	{
		DeleteObject(obj);
		return NULL;
	}

	m_ObjectTypes[0].m_Objects[key] = obj;

	return obj;
}


void CObjectManager::AddObjectType(const char* name)
{
	m_ObjectTypes.resize(m_ObjectTypes.size()+1);
	SObjectType& type=m_ObjectTypes.back();
	type.m_Name=name;
	type.m_Index=(int)m_ObjectTypes.size()-1;
}


void CObjectManager::AddObject(ObjectKey& key, CObjectEntry* entry, int type)
{
	debug_assert((uint)type<m_ObjectTypes.size());
	m_ObjectTypes[type].m_Objects.insert(std::make_pair(key, entry));
}

void CObjectManager::DeleteObject(CObjectEntry* entry)
{
	std::map<ObjectKey, CObjectEntry*>& objects = m_ObjectTypes[entry->m_Type].m_Objects;

	for (std::map<ObjectKey, CObjectEntry*>::iterator it = objects.begin(); it != objects.end(); )
		if (it->second == entry)
			objects.erase(it++);
		else
			++it;

	delete entry;
}


int CObjectManager::LoadObjects()
{
	AddObjectType("");
	return 0;
}

void CObjectManager::UnloadObjects()
{
	for (size_t i = 0; i < m_ObjectTypes.size(); i++) {
		std::for_each(
			m_ObjectTypes[i].m_Objects.begin(),
			m_ObjectTypes[i].m_Objects.end(),
			delete_pair_2nd<ObjectKey, CObjectEntry*>
		);
		std::for_each(
			m_ObjectTypes[i].m_ObjectBases.begin(),
			m_ObjectTypes[i].m_ObjectBases.end(),
			delete_pair_2nd<CStr, CObjectBase*>
		);
	}
	m_ObjectTypes.clear();

	delete m_SelectedThing;
	m_SelectedThing = NULL;
}



static void GetObjectName_ThunkCb(const char* path, const DirEnt* UNUSED(ent), void* context)
{
	std::vector<CStr>* names = (std::vector<CStr>*)context;
	CStr name (path);
	names->push_back(name.AfterFirst("actors/"));
}

void CObjectManager::GetAllObjectNames(std::vector<CStr>& names)
{
	VFSUtil::EnumDirEnts("art/actors/", VFSUtil::RECURSIVE, "*.xml",
		GetObjectName_ThunkCb, &names);
}

void CObjectManager::GetPropObjectNames(std::vector<CStr>& names)
{
	VFSUtil::EnumDirEnts("art/actors/props/", VFSUtil::RECURSIVE, "*.xml",
		GetObjectName_ThunkCb, &names);
}
