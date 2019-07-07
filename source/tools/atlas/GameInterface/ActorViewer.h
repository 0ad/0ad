/* Copyright (C) 2019 Wildfire Games.
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

#include "ps/CStr.h"
#include "simulation2/helpers/Player.h"
#include "simulation2/system/Entity.h"

struct ActorViewerImpl;
struct SColor4ub;
class CSimulation2;

class ActorViewer
{
	NONCOPYABLE(ActorViewer);
public:
	ActorViewer();
	~ActorViewer();

	CSimulation2* GetSimulation2();
	entity_id_t GetEntity();
	void SetActor(const CStrW& id, const CStr& animation, player_id_t playerID);
	void SetEnabled(bool enabled);
	void UnloadObjects();
	void SetBackgroundColor(const SColor4ub& color);
	void SetWalkEnabled(bool enabled);
	void SetGroundEnabled(bool enabled);
	void SetWaterEnabled(bool enabled);
	void SetShadowsEnabled(bool enabled);
	void SetStatsEnabled(bool enabled);
	void SetBoundingBoxesEnabled(bool enabled);
	void SetAxesMarkerEnabled(bool enabled);
	void SetPropPointsMode(int mode);
	void Render();
	void Update(float simFrameLength, float realFrameLength);

private:
	ActorViewerImpl& m;
};

#endif // INCLUDED_ACTORVIEWER
