#ifndef INCLUDED_TRASTARENGINE
#define INCLUDED_TRASTARENGINE

#ifdef USE_DCDT

#include "AStarEngine.h"

class CTRAStarEngine :
	public CAStarEngine
{
public:
	CTRAStarEngine(void);
	~CTRAStarEngine(void);

	SeDcdt dcdtPathfinder;;

	bool FindPath( const CVector2D& src, const CVector2D& dest, HEntity entity,  SeDcdt& dcdtPathfinder,float radius=0.0f );
};

#endif // USE_DCDT

#endif
