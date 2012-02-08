/* Copyright (C) 2012 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "ObjectManager.h"

#include "graphics/ObjectBase.h"
#include "graphics/ObjectEntry.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/Filesystem.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpVisual.h"

template<typename T, typename S>
static void delete_pair_2nd(std::pair<T,S> v)
{
	delete v.second;
}

template<typename T>
struct second_equals
{
	T x;
	second_equals(const T& x) : x(x) {}
	template<typename S> bool operator()(const S& v) { return v.second == x; }
};

bool CObjectManager::ObjectKey::operator< (const CObjectManager::ObjectKey& a) const
{
	if (ActorName < a.ActorName)
		return true;
	else if (ActorName > a.ActorName)
		return false;
	else
		return ActorVariation < a.ActorVariation;
}

static Status ReloadChangedFileCB(void* param, const VfsPath& path)
{
	return static_cast<CObjectManager*>(param)->ReloadChangedFile(path);
}

CObjectManager::CObjectManager(CMeshManager& meshManager, CSkeletonAnimManager& skeletonAnimManager, CSimulation2& simulation)
: m_MeshManager(meshManager), m_SkeletonAnimManager(skeletonAnimManager), m_Simulation(simulation)
{
	RegisterFileReloadFunc(ReloadChangedFileCB, this);
}

CObjectManager::~CObjectManager()
{
	UnloadObjects();

	UnregisterFileReloadFunc(ReloadChangedFileCB, this);
}


CObjectBase* CObjectManager::FindObjectBase(const CStrW& objectname)
{
	ENSURE(!objectname.empty());

	// See if the base type has been loaded yet:

	std::map<CStrW, CObjectBase*>::iterator it = m_ObjectBases.find(objectname);
	if (it != m_ObjectBases.end())
		return it->second;

	// Not already loaded, so try to load it:

	CObjectBase* obj = new CObjectBase(*this);

	VfsPath pathname = VfsPath("art/actors/") / objectname;

	if (obj->Load(pathname))
	{
		m_ObjectBases[objectname] = obj;
		return obj;
	}
	else
		delete obj;

	LOGERROR(L"CObjectManager::FindObjectBase(): Cannot find object '%ls'", objectname.c_str());

	return 0;
}

CObjectEntry* CObjectManager::FindObject(const CStrW& objname)
{
	std::vector<std::set<CStr> > selections; // TODO - should this really be empty?
	return FindObjectVariation(objname, selections);
}

CObjectEntry* CObjectManager::FindObjectVariation(const CStrW& objname, const std::vector<std::set<CStr> >& selections)
{
	CObjectBase* base = FindObjectBase(objname);

	if (! base)
		return NULL;

	return FindObjectVariation(base, selections);
}

CObjectEntry* CObjectManager::FindObjectVariation(CObjectBase* base, const std::vector<std::set<CStr> >& selections)
{
	PROFILE("object variation loading");

	// Look to see whether this particular variation has already been loaded

	std::vector<u8> choices = base->CalculateVariationKey(selections);
	ObjectKey key (base->m_Pathname.string(), choices);

	std::map<ObjectKey, CObjectEntry*>::iterator it = m_Objects.find(key);
	if (it != m_Objects.end() && !it->second->m_Outdated)
		return it->second;

	// If it hasn't been loaded, load it now

	// TODO: If there was an existing ObjectEntry, but it's outdated (due to hotloading),
	// we'll get a memory leak when replacing its entry in m_Objects. The problem is
	// some CUnits might still have a pointer to the old ObjectEntry so we probably can't
	// just delete it now. Probably we need to redesign the caching/hotloading system so it
	// makes more sense (e.g. use shared_ptr); for now I'll just leak, to avoid making the logic
	// more complex than it is already is, since this only matters for the rare case of hotloading.

	CObjectEntry* obj = new CObjectEntry(base); // TODO: type ?

	// TODO (for some efficiency): use the pre-calculated choices for this object,
	// which has already worked out what to do for props, instead of passing the
	// selections into BuildVariation and having it recalculate the props' choices.

	if (! obj->BuildVariation(selections, choices, *this))
	{
		DeleteObject(obj);
		return NULL;
	}

	m_Objects[key] = obj;

	return obj;
}

CTerrain* CObjectManager::GetTerrain()
{
	CmpPtr<ICmpTerrain> cmpTerrain(m_Simulation, SYSTEM_ENTITY);
	if (!cmpTerrain)
		return NULL;
	return cmpTerrain->GetCTerrain();
}

void CObjectManager::DeleteObject(CObjectEntry* entry)
{
	std::map<ObjectKey, CObjectEntry*>::iterator it;
	while (m_Objects.end() != (it = find_if(m_Objects.begin(), m_Objects.end(), second_equals<CObjectEntry*>(entry))))
		m_Objects.erase(it);

	delete entry;
}


void CObjectManager::UnloadObjects()
{
	std::for_each(
		m_Objects.begin(),
		m_Objects.end(),
		delete_pair_2nd<ObjectKey, CObjectEntry*>
	);
	m_Objects.clear();

	std::for_each(
		m_ObjectBases.begin(),
		m_ObjectBases.end(),
		delete_pair_2nd<CStrW, CObjectBase*>
	);
	m_ObjectBases.clear();
}

Status CObjectManager::ReloadChangedFile(const VfsPath& path)
{
	// Mark old entries as outdated so we don't reload them from the cache
	for (std::map<ObjectKey, CObjectEntry*>::iterator it = m_Objects.begin(); it != m_Objects.end(); ++it)
		if (it->second->m_Base->UsesFile(path))
			it->second->m_Outdated = true;

	// Reload actors that use a changed object
	for (std::map<CStrW, CObjectBase*>::iterator it = m_ObjectBases.begin(); it != m_ObjectBases.end(); ++it)
	{
		if (it->second->UsesFile(path))
		{
			it->second->Reload();

			// Slightly ugly hack: The graphics system doesn't preserve enough information to regenerate the
			// object with all correct variations, and we don't want to waste space storing it just for the
			// rare occurrence of hotloading, so we'll tell the component (which does preserve the information)
			// to do the reloading itself
			const CSimulation2::InterfaceListUnordered& cmps = m_Simulation.GetEntitiesWithInterfaceUnordered(IID_Visual);
			for (CSimulation2::InterfaceListUnordered::const_iterator eit = cmps.begin(); eit != cmps.end(); ++eit)
				static_cast<ICmpVisual*>(eit->second)->Hotload(it->first);
		}
	}

	return INFO::OK;
}
