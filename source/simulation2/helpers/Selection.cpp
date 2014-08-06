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

#include "Selection.h"

#include "graphics/Camera.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpIdentity.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/components/ICmpSelectable.h"
#include "simulation2/components/ICmpVisual.h"
#include "simulation2/helpers/Spatial.h"
#include "ps/CLogger.h"
#include "ps/Profiler2.h"

std::vector<entity_id_t> EntitySelection::PickEntitiesAtPoint(CSimulation2& simulation, const CCamera& camera, int screenX, int screenY, player_id_t player, bool allowEditorSelectables, int range)
{
	PROFILE2("PickEntitiesAtPoint");
	CVector3D origin, dir;
	camera.BuildCameraRay(screenX, screenY, origin, dir);

	CmpPtr<ICmpRangeManager> cmpRangeManager(simulation, SYSTEM_ENTITY);
	ENSURE(cmpRangeManager);

	/* We try to approximate where the mouse is hovering by drawing a ray from
	 * the center of the camera and through the mouse then taking the position
	 * at which the ray intersects the terrain.                               */
	// TODO: Do this smarter without being slow.
	CVector3D pos3d = camera.GetWorldCoordinates(screenX, screenY, true);
	// Change the position to 2D by removing the terrain height.
	CFixedVector2D pos(fixed::FromFloat(pos3d.X), fixed::FromFloat(pos3d.Z));

	// Get a rough group of entities using our approximated origin.
	std::vector<entity_id_t> ents;
	cmpRangeManager->GetSubdivision()->GetNear(ents, pos, entity_pos_t::FromInt(range));

	// Filter for relevent entities and calculate precise distances.
	std::vector<std::pair<float, entity_id_t> > hits; // (dist^2, entity) pairs
	for (size_t i = 0; i < ents.size(); ++i)
	{
		CmpPtr<ICmpSelectable> cmpSelectable(simulation, ents[i]);
		if (!cmpSelectable)
			continue;

		CEntityHandle handle = cmpSelectable->GetEntityHandle();

		// Check if this entity is only selectable in Atlas
		if (!allowEditorSelectables && cmpSelectable->IsEditorOnly())
			continue;

		// Ignore entities hidden by LOS (or otherwise hidden, e.g. when not IsInWorld)
		if (cmpRangeManager->GetLosVisibility(handle, player) == ICmpRangeManager::VIS_HIDDEN)
			continue;

		CmpPtr<ICmpVisual> cmpVisual(handle);
		if (!cmpVisual)
			continue;

		CVector3D center;
		float tmin, tmax;

		CBoundingBoxOriented selectionBox = cmpVisual->GetSelectionBox();
		if (selectionBox.IsEmpty())
		{
			if (!allowEditorSelectables)
				continue;

			// Fall back to using old AABB selection method for decals
			//	see: http://trac.wildfiregames.com/ticket/1032
			CBoundingBoxAligned aABBox = cmpVisual->GetBounds();
			if (aABBox.IsEmpty())
				continue;

			if (!aABBox.RayIntersect(origin, dir, tmin, tmax))
				continue;

			aABBox.GetCentre(center);
		}
		else
		{
			if (!selectionBox.RayIntersect(origin, dir, tmin, tmax))
				continue;

			center = selectionBox.m_Center;
		}

		// Find the perpendicular distance from the object's centre to the picker ray
		float dist2;
		CVector3D closest = origin + dir * (center - origin).Dot(dir);
		dist2 = (closest - center).LengthSquared();

		hits.push_back(std::make_pair(dist2, ents[i]));
	}

	// Sort hits by distance
	std::sort(hits.begin(), hits.end()); // lexicographic comparison

	// Extract the entity IDs
	std::vector<entity_id_t> hitEnts;
	hitEnts.reserve(hits.size());
	for (size_t i = 0; i < hits.size(); ++i)
		hitEnts.push_back(hits[i].second);
	return hitEnts;
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

	const CSimulation2::InterfaceListUnordered& ents = simulation.GetEntitiesWithInterfaceUnordered(IID_Selectable);
	for (CSimulation2::InterfaceListUnordered::const_iterator it = ents.begin(); it != ents.end(); ++it)
	{
		entity_id_t ent = it->first;
		CEntityHandle handle = it->second->GetEntityHandle();

		// Check if this entity is only selectable in Atlas
		if (static_cast<ICmpSelectable*>(it->second)->IsEditorOnly() && !allowEditorSelectables)
			continue;

		// Ignore entities hidden by LOS (or otherwise hidden, e.g. when not IsInWorld)
		if (cmpRangeManager->GetLosVisibility(handle, owner) == ICmpRangeManager::VIS_HIDDEN)
			continue;

		// Ignore entities not owned by 'owner'
		CmpPtr<ICmpOwnership> cmpOwnership(handle);
		if (owner != INVALID_PLAYER && (!cmpOwnership || cmpOwnership->GetOwner() != owner))
			continue;

		// Find the current interpolated model position.
		// (We just use the centre position and not the whole bounding box, because maybe
		// that's better for users trying to select objects in busy areas)

		CmpPtr<ICmpVisual> cmpVisual(handle);
		if (!cmpVisual)
			continue;

		CVector3D position = cmpVisual->GetPosition();

		// Reject if it's not on-screen (e.g. it's behind the camera)
		if (!camera.GetFrustum().IsPointVisible(position))
			continue;

		// Compare screen-space coordinates
		float x, y;
		camera.GetScreenCoordinates(position, x, y);
		int ix = (int)x;
		int iy = (int)y;
		if (sx0 <= ix && ix <= sx1 && sy0 <= iy && iy <= sy1)
			hitEnts.push_back(ent);
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
