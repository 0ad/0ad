#ifndef __SIMPLEPAINTERS_H__
#define __SIMPLEPAINTERS_H__

#include "areapainter.h"
#include "map.h"
#include "area.h"

class TerrainPainter : public AreaPainter {
public:
	std::string terrain;

	TerrainPainter(const std::string& terrain);
	void paint(Map* m, Area* a);
};

#endif