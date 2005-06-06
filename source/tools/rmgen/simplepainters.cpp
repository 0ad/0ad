#include "stdafx.h"
#include "simplepainters.h"
#include "random.h"
#include "rmgen.h"

using namespace std;

// TerrainPainter

TerrainPainter::TerrainPainter(Terrain* terrain)
{
	this->terrain = terrain;
}

void TerrainPainter::paint(Map* m, Area* a) 
{
	for (int i=0; i<a->points.size(); i++) {
		Point p = a->points[i];
		terrain->place(m, p.x, p.y);
	}
}
