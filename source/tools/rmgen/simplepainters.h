#ifndef __SIMPLEPAINTERS_H__
#define __SIMPLEPAINTERS_H__

#include "areapainter.h"
#include "map.h"
#include "area.h"
#include "terrain.h"

class TerrainPainter : public AreaPainter {
public:
	Terrain* terrain;

	TerrainPainter(Terrain* terrain);
	void paint(Map* m, Area* a);
};

#endif