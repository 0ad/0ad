#include "precompiled.h"

#include "PathfindEngine.h"
#include "PathfindSparse.h"

void CPathfindEngine::requestPath( HEntity entity, const CVector2D& destination )
{
	pathSparse( entity, destination );
}
