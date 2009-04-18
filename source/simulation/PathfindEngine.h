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

// PathfindEngine.h
// 
// The pathfinding engine singleton.
//
// Usage: g_Pathfinder.RequestPath( HEntity me, float x, float y );
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#ifndef INCLUDED_PATHFINDENGINE
#define INCLUDED_PATHFINDENGINE

#include "ps/Singleton.h"
#include "EntityHandles.h"
#include "ps/Vector2D.h"
#include "AStarEngine.h"

#ifdef USE_DCDT
# include "dcdt/se/se_dcdt.h"
# include "TRAStarEngine.h"
class TriangulationTerrainOverlay;
#endif // USE_DCDT

#define g_Pathfinder CPathfindEngine::GetSingleton()

class CEntityOrder;

enum EPathType
{
	PF_STANDARD,
	PF_ATTACK_MELEE,
};


class CPathfindEngine : public Singleton<CPathfindEngine>
{
	NONCOPYABLE(CPathfindEngine);

public:
#ifdef USE_DCDT
	//Kai: added for dcdt 
	const float OABBBOUNDREDUCTION  ; 
	const float CIRCLEBOUNDREDUCTION  ; 
	const float RADIUSINCREMENT  ; 
	
	
	
	static SrArray<SeBase*> processing;
	bool dcdtInitialized;

	SeDcdt dcdtPathfinder;
	void initBoundary();
	void insertObstacles();
	void drawTriangulation();

	//Kai:added tile overlay for pathfinding
	TriangulationTerrainOverlay* triangulationOverlay;
#endif

	CPathfindEngine();
	~CPathfindEngine();

	void RequestPath( HEntity entity, const CVector2D& destination, 
		CEntityOrder::EOrderSource orderSource );

	void RequestLowLevelPath( HEntity entity, const CVector2D& destination, bool contact, 
		float radius, CEntityOrder::EOrderSource orderSource );

	void RequestContactPath( HEntity entity, CEntityOrder* current, float range );
	bool RequestAvoidPath( HEntity entity, CEntityOrder* current, float avoidRange );

#ifdef USE_DCDT
	void RequestTriangulationPath( HEntity entity, const CVector2D& destination, bool contact, 
		float radius, CEntityOrder::EOrderSource orderSource );
#endif

private:
	CAStarEngineLowLevel mLowPathfinder;

#ifdef USE_DCDT
	CTRAStarEngine mTriangulationPathfinder;
#endif
};

#endif
