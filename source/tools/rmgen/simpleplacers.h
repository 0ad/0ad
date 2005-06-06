#ifndef __SIMPLEPLACERS_H__
#define __SIMPLEPLACERS_H__

#include "areaplacer.h"
#include "centeredplacer.h"
#include "map.h"
#include "constraint.h"

class ExactPlacer: public AreaPlacer {
private:
	CenteredPlacer* centeredPlacer;
	int x, y;
public:
	ExactPlacer(CenteredPlacer* centeredPlacer, int x, int y);
	virtual bool place(Map* m, Constraint* constr, std::vector<Point>& ret);
	~ExactPlacer();
};

class MultiPlacer: public AreaPlacer {
private:
	CenteredPlacer* centeredPlacer;
	int numToPlace, maxFail;
public:
	MultiPlacer(CenteredPlacer* centeredPlacer, int numToPlace, int maxFail);
	virtual bool place(Map* m, Constraint* constr, std::vector<Point>& ret);
	~MultiPlacer();
};

#endif