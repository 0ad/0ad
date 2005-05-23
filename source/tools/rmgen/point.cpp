#include "stdafx.h"
#include "point.h"

Point::Point(void)
{
	x = y = 0;
}

Point::Point(int x_, int y_) : x(x_), y(y_) 
{
}

Point::~Point(void)
{
}

bool Point::operator <(const Point& p) const {
	return x<p.x || (x==p.x && y<p.y);
}