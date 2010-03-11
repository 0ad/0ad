/* Copyright (C) 2010 Wildfire Games.
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
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpSelectable.h"
#include "simulation2/components/ICmpVisual.h"

std::vector<entity_id_t> EntitySelection::PickEntitiesAtPoint(CSimulation2& simulation, CCamera& camera, int screenX, int screenY)
{
	CVector3D origin, dir;
	camera.BuildCameraRay(screenX, screenY, origin, dir);

	std::vector<std::pair<float, entity_id_t> > hits; // (dist^2, entity) pairs

	const CSimulation2::InterfaceList& ents = simulation.GetEntitiesWithInterface(IID_Selectable);
	for (CSimulation2::InterfaceList::const_iterator it = ents.begin(); it != ents.end(); ++it)
	{
		entity_id_t ent = it->first;

		// TODO: ought to filter out units hidden by current player's LOS

		CmpPtr<ICmpVisual> cmpVisual(simulation.GetSimContext(), ent);
		if (cmpVisual.null())
			continue;

		CBound bounds = cmpVisual->GetBounds();

		float tmin, tmax;
		if (!bounds.RayIntersect(origin, dir, tmin, tmax))
			continue;

		// Find the perpendicular distance from the object's centre to the picker ray

		CVector3D centre;
		bounds.GetCentre(centre);

		CVector3D closest = origin + dir * (centre - origin).Dot(dir);
		float dist2 = (closest - centre).LengthSquared();

		hits.push_back(std::make_pair(dist2, ent));
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

std::vector<entity_id_t> EntitySelection::PickEntitiesInRect(CSimulation2& simulation, CCamera& camera, int sx0, int sy0, int sx1, int sy1, int owner)
{
	// Make sure sx0 <= sx1, and sy0 <= sy1
	if (sx0 > sx1)
		std::swap(sx0, sx1);
	if (sy0 > sy1)
		std::swap(sy0, sy1);

	std::vector<entity_id_t> hitEnts;

	const CSimulation2::InterfaceList& ents = simulation.GetEntitiesWithInterface(IID_Selectable);
	for (CSimulation2::InterfaceList::const_iterator it = ents.begin(); it != ents.end(); ++it)
	{
		entity_id_t ent = it->first;

		// Ignore entities not owned by 'owner'
		CmpPtr<ICmpOwnership> cmpOwnership(simulation.GetSimContext(), ent);
		if (cmpOwnership.null() || cmpOwnership->GetOwner() != owner)
			continue;

		// Find the current interpolated model position.
		// (We just use the centre position and not the whole bounding box, because maybe
		// that's better for users trying to select objects in busy areas)

		CmpPtr<ICmpVisual> cmpVisual(simulation.GetSimContext(), ent);
		if (cmpVisual.null())
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
