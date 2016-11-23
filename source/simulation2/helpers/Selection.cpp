/* Copyright (C) 2016 Wildfire Games.
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

#include "Selection.h"

#include "graphics/Camera.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpIdentity.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/components/ICmpSelectable.h"
#include "simulation2/components/ICmpVisual.h"
#include "simulation2/components/ICmpUnitRenderer.h"
#include "simulation2/system/ComponentManager.h"
#include "ps/CLogger.h"
#include "ps/Profiler2.h"

entity_id_t EntitySelection::PickEntityAtPoint(CSimulation2& simulation, const CCamera& camera, int screenX, int screenY, player_id_t player, bool allowEditorSelectables)
{	
	PROFILE2("PickEntityAtPoint");
	CVector3D origin, dir;
	camera.BuildCameraRay(screenX, screenY, origin, dir);
	
	CmpPtr<ICmpUnitRenderer> cmpUnitRenderer(simulation.GetSimContext().GetSystemEntity());
	ENSURE(cmpUnitRenderer);

	std::vector<std::pair<CEntityHandle, CVector3D> > entities;
	cmpUnitRenderer->PickAllEntitiesAtPoint(entities, origin, dir, allowEditorSelectables);
	if (entities.empty())
		return INVALID_ENTITY;
	
	// Filter for relevent entities in the list of candidates (all entities below the mouse)
	std::vector<std::pair<float, CEntityHandle> > hits; // (dist^2, entity) pairs
	for (size_t i = 0; i < entities.size(); ++i)
	{
		// Find the perpendicular distance from the object's centre to the picker ray
		float dist2;
		const CVector3D center = entities[i].second;
		CVector3D closest = origin + dir * (center - origin).Dot(dir);
		dist2 = (closest - center).LengthSquared();
		hits.emplace_back(dist2, entities[i].first);
	}

	// Sort hits by distance
	std::sort(hits.begin(), hits.end(),
		[](const std::pair<float, CEntityHandle>& a, const std::pair<float, CEntityHandle>& b) {
			return a.first < b.first;
		});
	
	CmpPtr<ICmpRangeManager> cmpRangeManager(simulation, SYSTEM_ENTITY);
	ENSURE(cmpRangeManager);
	
	for (size_t i = 0; i < hits.size(); ++i)
	{
		const CEntityHandle& handle = hits[i].second;
		
		CmpPtr<ICmpSelectable> cmpSelectable(handle);
		if (!cmpSelectable)
			continue;

		// Check if this entity is only selectable in Atlas
		if (!allowEditorSelectables && cmpSelectable->IsEditorOnly())
			continue;

		// Ignore entities hidden by LOS (or otherwise hidden, e.g. when not IsInWorld)
		if (cmpRangeManager->GetLosVisibility(handle, player) == ICmpRangeManager::VIS_HIDDEN)
			continue;
			
		return handle.GetId();
	}
	return INVALID_ENTITY;
}

/**
 * Returns true if the given entity is visible to the given player and visible in the given screen area.
 * If the entity is a decorative, the function will only return true if allowEditorSelectables.
 */
static bool CheckEntityVisibleAndInRect(CEntityHandle handle, CmpPtr<ICmpRangeManager> cmpRangeManager, const CCamera& camera, int sx0, int sy0, int sx1, int sy1, player_id_t owner, bool allowEditorSelectables)
{
	// Check if this entity is only selectable in Atlas
	CmpPtr<ICmpSelectable> cmpSelectable(handle);
	if (!cmpSelectable || (!allowEditorSelectables && cmpSelectable->IsEditorOnly()))
		return false;

	// Ignore entities hidden by LOS (or otherwise hidden, e.g. when not IsInWorld)
	if (cmpRangeManager->GetLosVisibility(handle, owner) == ICmpRangeManager::VIS_HIDDEN)
		return false;

	// Find the current interpolated model position.
	// (We just use the centre position and not the whole bounding box, because maybe
	// that's better for users trying to select objects in busy areas)

	CmpPtr<ICmpVisual> cmpVisual(handle);
	if (!cmpVisual)
		return false;

	CVector3D position = cmpVisual->GetPosition();

	// Reject if it's not on-screen (e.g. it's behind the camera)
	if (!camera.GetFrustum().IsPointVisible(position))
		return false;

	// Compare screen-space coordinates
	float x, y;
	camera.GetScreenCoordinates(position, x, y);
	int ix = (int)x;
	int iy = (int)y;
	return sx0 <= ix && ix <= sx1 && sy0 <= iy && iy <= sy1;
}

std::vector<entity_id_t> EntitySelection::PickEntitiesInRect(CSimulation2& simulation, const CCamera& camera, int sx0, int sy0, int sx1, int sy1, player_id_t owner, bool allowEditorSelectables)
{
	PROFILE2("PickEntitiesInRect");
	// Make sure sx0 <= sx1, and sy0 <= sy1
	if (sx0 > sx1)
		std::swap(sx0, sx1);
	if (sy0 > sy1)
		std::swap(sy0, sy1);

	CmpPtr<ICmpRangeManager> cmpRangeManager(simulation, SYSTEM_ENTITY);
	ENSURE(cmpRangeManager);

	std::vector<entity_id_t> hitEnts;

	if (owner != INVALID_PLAYER)
	{
		CComponentManager& componentManager = simulation.GetSimContext().GetComponentManager();
		std::vector<entity_id_t> ents = cmpRangeManager->GetEntitiesByPlayer(owner);
		for (std::vector<entity_id_t>::iterator it = ents.begin(); it != ents.end(); ++it)
		{
			if (CheckEntityVisibleAndInRect(componentManager.LookupEntityHandle(*it), cmpRangeManager, camera, sx0, sy0, sx1, sy1, owner, allowEditorSelectables))
				hitEnts.push_back(*it);
		}
	}
	else // owner == INVALID_PLAYER; Used when selecting units in Atlas or other mods that allow all kinds of selectables to be selected.
	{
		const CSimulation2::InterfaceListUnordered& selectableEnts = simulation.GetEntitiesWithInterfaceUnordered(IID_Selectable);
		for (CSimulation2::InterfaceListUnordered::const_iterator it = selectableEnts.begin(); it != selectableEnts.end(); ++it)
		{
			if (CheckEntityVisibleAndInRect(it->second->GetEntityHandle(), cmpRangeManager, camera, sx0, sy0, sx1, sy1, owner, allowEditorSelectables))
				hitEnts.push_back(it->first);
		}
	}

	return hitEnts;
}

std::vector<entity_id_t> EntitySelection::PickSimilarEntities(CSimulation2& simulation, const CCamera& camera,
	const std::string& templateName, player_id_t owner, bool includeOffScreen, bool matchRank,
	bool allowEditorSelectables, bool allowFoundations)
{
	PROFILE2("PickSimilarEntities");
	CmpPtr<ICmpTemplateManager> cmpTemplateManager(simulation, SYSTEM_ENTITY);
	CmpPtr<ICmpRangeManager> cmpRangeManager(simulation, SYSTEM_ENTITY);

	std::vector<entity_id_t> hitEnts;

 	const CSimulation2::InterfaceListUnordered& ents = simulation.GetEntitiesWithInterfaceUnordered(IID_Selectable);
	for (CSimulation2::InterfaceListUnordered::const_iterator it = ents.begin(); it != ents.end(); ++it)
	{
 		entity_id_t ent = it->first;
		CEntityHandle handle = it->second->GetEntityHandle();

		// Check if this entity is only selectable in Atlas
		if (static_cast<ICmpSelectable*>(it->second)->IsEditorOnly() && !allowEditorSelectables)
			continue;

		if (matchRank)
		{
			// Exact template name matching, optionally also allowing foundations
			std::string curTemplateName = cmpTemplateManager->GetCurrentTemplateName(ent);
			bool matches = (curTemplateName == templateName ||
			                (allowFoundations && curTemplateName.substr(0, 11) == "foundation|" && curTemplateName.substr(11) == templateName));
			if (!matches)
				continue;
		}

		// Ignore entities hidden by LOS (or otherwise hidden, e.g. when not IsInWorld)
		// In this case, the checking is done to avoid selecting garrisoned units
		if (cmpRangeManager->GetLosVisibility(handle, owner) == ICmpRangeManager::VIS_HIDDEN)
			continue;

		// Ignore entities not owned by 'owner'
		CmpPtr<ICmpOwnership> cmpOwnership(simulation.GetSimContext(), ent);
		if (owner != INVALID_PLAYER && (!cmpOwnership || cmpOwnership->GetOwner() != owner))
			continue;

		// Ignore off screen entities
		if (!includeOffScreen)
		{
 			// Find the current interpolated model position.
			CmpPtr<ICmpVisual> cmpVisual(simulation.GetSimContext(), ent);
			if (!cmpVisual)
				continue;
			CVector3D position = cmpVisual->GetPosition();

			// Reject if it's not on-screen (e.g. it's behind the camera)
			if (!camera.GetFrustum().IsPointVisible(position))
				continue;
		}

		if (!matchRank)
		{
			// Match by selection group name
			// (This is relatively expensive since it involves script calls, so do it after all other tests)
			CmpPtr<ICmpIdentity> cmpIdentity(simulation.GetSimContext(), ent);
			if (!cmpIdentity || cmpIdentity->GetSelectionGroupName() != templateName)
				continue;
		}

 		hitEnts.push_back(ent);
 	}

 	return hitEnts;
}
