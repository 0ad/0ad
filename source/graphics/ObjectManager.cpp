#include "precompiled.h"

#include "ObjectManager.h"
#include <algorithm>
#include "CLogger.h"
#include "lib/res/res.h"
#include "timer.h"
#include "VFSUtil.h"
#include "ObjectBase.h"
#include "ObjectEntry.h"

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

CObjectManager::CObjectManager() : m_SelectedEntity(NULL)
{
	m_ObjectTypes.reserve(32);
}

template<typename T, typename S> void delete_pair_2nd(std::pair<T,S> v) { delete v.second; }

CObjectManager::~CObjectManager()
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

	for (uint k = 0; k < m_ObjectTypes.size(); k++) {

		CStr filename ("art/actors/");
		filename += objectname;

		CObjectBase* obj = new CObjectBase();

		if (obj->Load(filename))
		{
			m_ObjectTypes[k].m_ObjectBases[objectname] = obj;
			return obj;
		}
		else
			delete obj;
	}

	LOG(ERROR, LOG_CATEGORY, "CObjectManager::FindObject(): Cannot find object '%s'", objectname);

	return 0;
}

CObjectEntry* CObjectManager::FindObject(const char* objname)
{
	CObjectBase* base = FindObjectBase(objname);

	if (! base)
		return NULL;

	std::set<CStr> choices;
	// TODO: Fill in these choices from somewhere, e.g.:
	//choices.insert("whatever");

	CObjectBase::variation_key var;
	base->CalculateVariation(choices, var);

	// Look to see whether this particular variation has already been loaded

	ObjectKey key (CStr(objname), var);

	std::map<ObjectKey, CObjectEntry*>::iterator it = m_ObjectTypes[0].m_Objects.find(key);
	if (it != m_ObjectTypes[0].m_Objects.end())
		return it->second;

	// If it hasn't been loaded, load it now

	CObjectEntry* obj = new CObjectEntry(0, base); // TODO: type ???

	obj->ApplyRandomVariant(var);

	if (! obj->BuildModel())
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
	assert((uint)type<m_ObjectTypes.size());
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


void CObjectManager::AddObjectBase(CObjectBase* base)
{
	m_ObjectTypes[0].m_ObjectBases.insert(make_pair(base->m_FileName, base));
}

void CObjectManager::DeleteObjectBase(CObjectBase* base)
{
	std::map<CStr, CObjectBase*>& objects = m_ObjectTypes[0].m_ObjectBases;

	for (std::map<CStr, CObjectBase*>::iterator it = objects.begin(); it != objects.end(); )
		if (it->second == base)
			objects.erase(it++);
		else
			++it;

	delete base;
}


void CObjectManager::LoadObjects()
{
	AddObjectType("");
}
