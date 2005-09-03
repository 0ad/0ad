#ifndef __SIMPLEPAINTERS_H__
#define __SIMPLEPAINTERS_H__

#include "areapainter.h"
#include "map.h"
#include "area.h"
#include "terrain.h"
#include "tileclass.h"

class TerrainPainter : public AreaPainter {
	Terrain* terrain;
public:
	TerrainPainter(Terrain* terrain);
	virtual void paint(Map* m, Area* a);
};

class ElevationPainter : public AreaPainter {
	float elevation;
public:
	ElevationPainter(float elevation);
	virtual void paint(Map* m, Area* a);
};

class TileClassPainter : public AreaPainter {
	TileClass* tileClass;
public:
	TileClassPainter(TileClass* tc);
	virtual void paint(Map* m, Area* a);
};

class MultiPainter : public AreaPainter {
	std::vector<AreaPainter*> painters;
public:
	MultiPainter(const std::vector<AreaPainter*>& painters);
	virtual void paint(Map* m, Area* a);
};

#endif