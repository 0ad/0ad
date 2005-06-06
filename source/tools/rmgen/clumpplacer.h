#ifndef __CLUMPPLACER_H__
#define __CLUMPPLACER_H__

#include "centeredplacer.h"
#include "map.h"

class ClumpPlacer : public CenteredPlacer
{
private:
	float size;
	float coherence;
	float smoothness;
public:
	virtual bool place(Map* m, Constraint* constr, std::vector<Point>& ret, int x, int y);

	ClumpPlacer(float size, float coherence, float smoothness);
	virtual ~ClumpPlacer();
};

#endif