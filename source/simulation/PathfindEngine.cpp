#include "precompiled.h"

#include "PathfindEngine.h"
#include "PathfindSparse.h"
#include "ConfigDB.h"

CPathfindEngine::CPathfindEngine()
{
	CConfigValue* sparseDepth = g_ConfigDB.GetValue( CFG_USER, "pathfind.sparse.recursiondepth" );
	if( sparseDepth )
		sparseDepth->GetInt( SPF_RECURSION_DEPTH );
}

void CPathfindEngine::requestPath( HEntity entity, const CVector2D& destination )
{
	pathSparse( entity, destination );
}
