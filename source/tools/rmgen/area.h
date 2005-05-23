#pragma once

#include "point.h"

class Area
{
public:
	std::vector<Point> points;

	Area(void);
	Area(const std::vector<Point>& points);
	~Area(void);
};
