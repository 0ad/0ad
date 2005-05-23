#ifndef __CENTEREDPLACER_H__
#define __CENTEREDPLACER_H__

#include "point.h"
#include "constraint.h"

class CenteredPlacer
{
public:
	virtual bool place(class Map* m, Constraint* constr, std::vector<Point>& ret, int x, int y) = 0;

	CenteredPlacer(void);
	virtual ~CenteredPlacer(void);
};

#endif