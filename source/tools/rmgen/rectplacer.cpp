#include "stdafx.h"
#include "rectplacer.h"
#include "map.h"

RectPlacer::RectPlacer(int x1, int y1, int x2, int y2)
{
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
}

RectPlacer::~RectPlacer(void)
{
}

bool RectPlacer::place(Map* m, Constraint* constr, std::vector<Point>& ret) {
	for(int x=x1; x<x2; x++) {
		for(int y=y1; y<y2; y++) {
			if(m->validT(x,y) && constr->allows(m,x,y)) {
				ret.push_back(Point(x,y));
			}
			else {
				return false;
			}
		}
	}
	return true;
}
