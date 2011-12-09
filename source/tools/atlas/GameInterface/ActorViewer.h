/* Copyright (C) 2009 Wildfire Games.
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

#ifndef INCLUDED_ACTORVIEWER
#define INCLUDED_ACTORVIEWER

#include "simulation2/system/Entity.h"

struct ActorViewerImpl;
struct SColor4ub;
class CSimulation2;
class CStrW;

class ActorViewer
{
	NONCOPYABLE(ActorViewer);
public:
	ActorViewer();
	~ActorViewer();

	CSimulation2* GetSimulation2();
	entity_id_t GetEntity();
	void SetActor(const CStrW& id, const CStrW& animation);
	void UnloadObjects();
	void SetBackgroundColour(const SColor4ub& colour);
	void SetWalkEnabled(bool enabled);
	void SetGroundEnabled(bool enabled);
	void SetShadowsEnabled(bool enabled);
	void SetStatsEnabled(bool enabled);
	void SetBoundingBoxesEnabled(bool enabled);
	void SetAxesMarkerEnabled(bool enabled);
	void SetPropPointsEnabled(bool enabled);
	void Render();
	void Update(float dt);

private:
	ActorViewerImpl& m;
};

#endif // INCLUDED_ACTORVIEWER
