#include "precompiled.h"
#include "TRAStarEngine.h"

#ifdef USE_DCDT

CTRAStarEngine::CTRAStarEngine(void)
{
	
	mGoal = new AStarGoalLowLevel;
}

CTRAStarEngine::~CTRAStarEngine(void)
{
	delete mGoal;
}

bool CTRAStarEngine::FindPath(const CVector2D &src, const CVector2D &dest, HEntity UNUSED(entity), SeDcdt& dcdtPathfinder, float radius )
{
	bool found = dcdtPathfinder.SearchPathFast(src.x, src.y, dest.x, dest.y, radius);
	return found;
}

#endif // USE_DCDT
