// PathfindEngine.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// The pathfinding engine singleton.
//
// Usage: g_Pathfinder.requestPath( HEntity me, float x, float y );
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#ifndef PATHFIND_ENGINE_INCLUDED
#define PATHFIND_ENGINE_INCLUDED

#include "Singleton.h"
#include "EntityHandles.h"
#include "Vector2D.h"

#define g_Pathfinder CPathfindEngine::GetSingleton()

enum EPathType
{
	PF_STANDARD,
	PF_ATTACK_MELEE,
};

class CPathfindEngine : public Singleton<CPathfindEngine>
{
public:
	CPathfindEngine();
	void requestPath( HEntity entity, const CVector2D& destination );
	void requestContactPath( HEntity entity, HEntity target, int transition );
};

#endif
