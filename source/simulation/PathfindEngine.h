// PathfindEngine.h
//
// Last modified: 28 May 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
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

class CPathfindEngine : public Singleton<CPathfindEngine>
{
public:
	void requestPath( HEntity entity, const CVector2D& destination );
};

#endif
