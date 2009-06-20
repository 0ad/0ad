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

struct ActorViewerImpl;
struct SColor4ub;
class CUnit;
class CStrW;

class ActorViewer
{
	NONCOPYABLE(ActorViewer);
public:
	ActorViewer();
	~ActorViewer();

	void SetActor(const CStrW& id, const CStrW& animation);
	void UnloadObjects();
	CUnit* GetUnit();
	void SetBackgroundColour(const SColor4ub& colour);
	void SetWalkEnabled(bool enabled);
	void SetGroundEnabled(bool enabled);
	void SetShadowsEnabled(bool enabled);
	void SetStatsEnabled(bool enabled);
	void Render();
	void Update(float dt);
	
	// Returns whether there is a selected actor which has more than one
	// frame of animation
	bool HasAnimation() const;

private:
	ActorViewerImpl& m;
};

#endif // INCLUDED_ACTORVIEWER
