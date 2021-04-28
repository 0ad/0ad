/* Copyright (C) 2021 Wildfire Games.
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
#include "ps/ConfigDB.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpVisual.h"

bool CObjectManager::ObjectKey::operator< (const CObjectManager::ObjectKey& a) const
{
	if (ObjectBaseIdentifier < a.ObjectBaseIdentifier)
		return true;
	else if (ObjectBaseIdentifier > a.ObjectBaseIdentifier)
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

	m_QualityHook = std::make_unique<CConfigDBHook>(g_ConfigDB.RegisterHookAndCall("max_actor_quality", [this]() { ActorQualityChanged(); }));

	if (!CXeromyces::AddValidator(g_VFS, "actor", "art/actors/actor.rng"))
		LOGERROR("CObjectManager: failed to load actor grammar file 'art/actors/actor.rng'");
}

CObjectManager::~CObjectManager()
{
	UnloadObjects();

	UnregisterFileReloadFunc(ReloadChangedFileCB, this);
}

std::pair<bool, CActorDef&> CObjectManager::FindActorDef(const CStrW& actorName)
{
	ENSURE(!actorName.empty());

	decltype(m_ActorDefs)::iterator it = m_ActorDefs.find(actorName);
	if (it != m_ActorDefs.end() && !it->second.outdated)
		return { true, *it->second.obj };

	std::unique_ptr<CActorDef> actor = std::make_unique<CActorDef>(*this);

	VfsPath pathname = VfsPath("art/actors/") / actorName;

	bool success = true;
	if (!actor->Load(pathname))
	{
		// In case of failure, load a placeholder - we want to have an actor around for hotloading.
		// (this will leave garbage actors in the object manager if loading files with typos in the name,
		// but that's unlikely to be a large memory problem).
		LOGERROR("CObjectManager::FindActorDef(): Cannot find actor '%s'", utf8_from_wstring(actorName));
		actor->LoadErrorPlaceholder(pathname);
		success = false;
	}

	return { success, *m_ActorDefs.insert_or_assign(actorName, std::move(actor)).first->second.obj };
}

CObjectEntry* CObjectManager::FindObjectVariation(const CActorDef* actor, const std::vector<std::set<CStr>>& selections, uint32_t seed)
{
	if (!actor)
		return nullptr;

	const std::shared_ptr<CObjectBase>& base = actor->GetBase(m_QualityLevel);

	std::vector<const std::set<CStr>*> completeSelections;
	for (const std::set<CStr>& selectionSet : selections)
		completeSelections.emplace_back(&selectionSet);
	// To maintain a consistent look between quality levels, first complete with the highest-quality variants.
	// then complete again at the required quality level (since not all variants may be available).
	std::set<CStr> highQualitySelections = actor->GetBase(255)->CalculateRandomRemainingSelections(seed, selections);
	completeSelections.emplace_back(&highQualitySelections);
	// We don't have to pass the high-quality selections here because they have higher priority anyways.
	std::set<CStr> remainingSelections = base->CalculateRandomRemainingSelections(seed, selections);
	completeSelections.emplace_back(&remainingSelections);
	return FindObjectVariation(base, completeSelections);
}

CObjectEntry* CObjectManager::FindObjectVariation(const std::shared_ptr<CObjectBase>& base, const std::vector<const std::set<CStr>*>& completeSelections)
{
	PROFILE2("FindObjectVariation");

	// Look to see whether this particular variation has already been loaded
	std::vector<u8> choices = base->CalculateVariationKey(completeSelections);
	ObjectKey key (base->GetIdentifier(), choices);
	decltype(m_Objects)::iterator it = m_Objects.find(key);
	if (it != m_Objects.end() && !it->second.outdated)
		return it->second.obj.get();

	// If it hasn't been loaded, load it now.

	std::unique_ptr<CObjectEntry> obj = std::make_unique<CObjectEntry>(base, m_Simulation);

	// TODO (for some efficiency): use the pre-calculated choices for this object,
	// which has already worked out what to do for props, instead of passing the
	// selections into BuildVariation and having it recalculate the props' choices.

	if (!obj->BuildVariation(completeSelections, choices, *this))
		return nullptr;

	return m_Objects.insert_or_assign(key, std::move(obj)).first->second.obj.get();
}

CTerrain* CObjectManager::GetTerrain()
{
	CmpPtr<ICmpTerrain> cmpTerrain(m_Simulation, SYSTEM_ENTITY);
	if (!cmpTerrain)
		return NULL;
	return cmpTerrain->GetCTerrain();
}

void CObjectManager::UnloadObjects()
{
	m_Objects.clear();
	m_ActorDefs.clear();
}

Status CObjectManager::ReloadChangedFile(const VfsPath& path)
{
	// Mark old entries as outdated so we don't reload them from the cache
	for (std::pair<const ObjectKey, Hotloadable<CObjectEntry>>& object : m_Objects)
		if (!object.second.outdated && object.second.obj->m_Base->UsesFile(path))
			object.second.outdated = true;

	const CSimulation2::InterfaceListUnordered& cmps = m_Simulation.GetEntitiesWithInterfaceUnordered(IID_Visual);

	// Reload actors that use a changed object
	for (std::pair<const CStrW, Hotloadable<CActorDef>>& actor : m_ActorDefs)
	{
		if (!actor.second.outdated && actor.second.obj->UsesFile(path))
			actor.second.outdated = true;

		// Slightly ugly hack: The graphics system doesn't preserve enough information to regenerate the
		// object with all correct variations, and we don't want to waste space storing it just for the
		// rare occurrence of hotloading, so we'll tell the component (which does preserve the information)
		// to do the reloading itself
		for (CSimulation2::InterfaceListUnordered::const_iterator eit = cmps.begin(); eit != cmps.end(); ++eit)
			static_cast<ICmpVisual*>(eit->second)->Hotload(actor.first);
	}
	return INFO::OK;
}

void CObjectManager::ActorQualityChanged()
{
	int quality;
	CFG_GET_VAL("max_actor_quality", quality);
	if (quality == m_QualityLevel)
		return;

	m_QualityLevel = quality > 255 ? 255 : quality < 0 ? 0 : quality;

	// No need to reload entries or actors, but we do need to reload all units.
	const CSimulation2::InterfaceListUnordered& cmps = m_Simulation.GetEntitiesWithInterfaceUnordered(IID_Visual);
	for (CSimulation2::InterfaceListUnordered::const_iterator eit = cmps.begin(); eit != cmps.end(); ++eit)
		static_cast<ICmpVisual*>(eit->second)->Hotload();

	// Trigger an interpolate call - needed because the game is generally paused & models disappear otherwise.
	m_Simulation.Interpolate(0.f, 0.f, 0.f);
}
