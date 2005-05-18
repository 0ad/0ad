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
	delete m_SelectedThing;
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
	CObjectBase* base = FindObjectBase(objname);

	if (! base)
		return NULL;

	std::set<CStr> choices;
	// TODO: Fill in these choices from somewhere, e.g.:
	//choices.insert("whatever");

	CObjectBase::variation_key var;
	base->CalculateVariation(choices, var);

	CObjectBase::variation_key::iterator vars_it=var.begin();
	return FindObjectVariation(base, var, vars_it);
}

CObjectEntry* CObjectManager::FindObjectVariation(const char* objname, CObjectBase::variation_key vars, CObjectBase::variation_key::iterator& vars_it)
{
	CObjectBase* base = FindObjectBase(objname);

	if (! base)
		return NULL;

	return FindObjectVariation(base, vars, vars_it);
}

CObjectEntry* CObjectManager::FindObjectVariation(CObjectBase* base, CObjectBase::variation_key vars, CObjectBase::variation_key::iterator& vars_it)
{
	// Look to see whether this particular variation has already been loaded

	ObjectKey key (base->m_Name, vars);

	std::map<ObjectKey, CObjectEntry*>::iterator it = m_ObjectTypes[0].m_Objects.find(key);
	if (it != m_ObjectTypes[0].m_Objects.end())
		return it->second;

	// If it hasn't been loaded, load it now

	CObjectEntry* obj = new CObjectEntry(0, base); // TODO: type ?

	if (! obj->BuildRandomVariant(vars, vars_it))
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


int CObjectManager::LoadObjects()
{
	AddObjectType("");
	return 0;
}


//////////////////////////////////////////////////////////////////////////
// For ScEd:

static void GetObjectThunk(const char* path, const vfsDirEnt* ent, void* context)
{
	std::vector<CStr>* names = (std::vector<CStr>*)context;
	CStr name (path);
	names->push_back(name.AfterFirst("actors/"));
}
void CObjectManager::GetAllObjectNames(std::vector<CStr>& names)
{
	VFSUtil::EnumDirEnts("art/actors/", "*.xml", true, GetObjectThunk, &names);
}

struct CObjectThing_Entity : public CObjectThing
{
	CObjectThing_Entity(CBaseEntity* b)	: base(b), obj(g_ObjMan.FindObject((CStr)b->m_actorName)), ent(NULL) {}
	~CObjectThing_Entity() {}
	CBaseEntity* base;
	CEntity* ent;
	CObjectEntry* obj;
	void Create(CMatrix3D& transform, int playerID)
	{
		CVector3D orient = transform.GetIn();
		CVector3D position = transform.GetTranslation();
		ent = g_EntityManager.create(base, position, atan2(-orient.X, -orient.Z));
		ent->SetPlayer(g_Game->GetPlayer(playerID));
	}
	void SetTransform(CMatrix3D& transform)
	{
		CVector3D orient = transform.GetIn();
		CVector3D position = transform.GetTranslation();

		// This looks quite yucky, but nothing else seems to actually work:
		ent->m_position =
		ent->m_position_previous =
		ent->m_graphics_position = position;
		ent->teleport();
		ent->m_orientation =
		ent->m_orientation_previous =
		ent->m_graphics_orientation = atan2(-orient.X, -orient.Z);
		ent->reorient();
	}
	CObjectEntry* GetObjectEntry()
	{
		return obj;
	}
};
struct CObjectThing_Object : public CObjectThing
{
	CObjectThing_Object(CObjectEntry* o) : obj(o) {}
	~CObjectThing_Object() {}
	CObjectEntry* obj;
	CUnit* unit;
	void Create(CMatrix3D& transform, int playerID)
	{
		unit = new CUnit(obj, obj->m_Model->Clone());
		unit->GetModel()->SetTransform(transform);
		g_UnitMan.AddUnit(unit);
	}
	void SetTransform(CMatrix3D& transform)
	{
		unit->GetModel()->SetTransform(transform);
	}
	CObjectEntry* GetObjectEntry()
	{
		return obj;
	}
};

void CObjectManager::SetSelectedEntity(CBaseEntity* thing)
{
	delete m_SelectedThing;
	m_SelectedThing = (thing ? new CObjectThing_Entity(thing) : NULL);
}
void CObjectManager::SetSelectedObject(CObjectEntry* thing)
{
	delete m_SelectedThing;
	m_SelectedThing = (thing ? new CObjectThing_Object(thing) : NULL);
}
