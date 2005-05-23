#ifndef __RECTPLACER_H__
#define __RECTPLACER_H__

#include "areaplacer.h"
#include "map.h"

class RectPlacer :
	public AreaPlacer
{
public:
	int x1, y1, x2, y2;

	bool place(Map* m, Constraint* constr, std::vector<Point>& ret);

	RectPlacer(int x1, int y1, int x2, int y2);
	~RectPlacer(void);
};

#endif