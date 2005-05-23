#include "StdAfx.h"
#include ".\area.h"

Area::Area(void)
{
}

Area::Area(const std::vector<Point>& points) {
    this->points = points;
}

Area::~Area(void)
{
}
