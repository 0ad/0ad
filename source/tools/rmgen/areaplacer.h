#ifndef __AREAPLACER_H__
#define __AREAPLACER_H__

#include "point.h"
#include "constraint.h"

class AreaPlacer
{
public:
	virtual bool place(class Map* m, Constraint* constr, std::vector<Point>& ret) = 0;

	AreaPlacer(void);
	virtual ~AreaPlacer(void);
};

#endif