#ifndef __CLUMPPLACER_H__
#define __CLUMPPLACER_H__

#include "areaplacer.h"
#include "map.h"

class ClumpPlacer : public AreaPlacer
{
private:
	float size;
	float coherence;
	float smoothness;
	float failFraction;
	int x, y;
public:
	virtual bool place(Map* m, Constraint* constr, std::vector<Point>& ret);

	ClumpPlacer(float size, float coherence, float smoothness, float failFraction, int x, int y);
	virtual ~ClumpPlacer();
};

#endif