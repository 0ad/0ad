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

// ElevationPainter

ElevationPainter::ElevationPainter(float elevation)
{
	this->elevation = elevation;
}

void ElevationPainter::paint(Map* m, Area* a) 
{
	for (int i=0; i<a->points.size(); i++) {
		Point p = a->points[i];
		static const int DX[4] = {0,1,1,0};
		static const int DY[4] = {0,0,1,1};
		for(int j=0; j<4; j++) {
			m->height[p.x+DX[j]][p.y+DY[j]] = elevation;
		}
	}
}

// MultiPainter

MultiPainter::MultiPainter(const std::vector<AreaPainter*>& painters) 
{
	this->painters = painters;
}

void MultiPainter::paint(Map* m, Area* a) 
{
	for (int i=0; i<painters.size(); i++) {
		painters[i]->paint(m, a);
	}
}


// TileClassPainter

TileClassPainter::TileClassPainter(TileClass* tc)
{
	this->tileClass = tc;
}

void TileClassPainter::paint(Map* m, Area* a) 
{
	for (int i=0; i<a->points.size(); i++) {
		Point p = a->points[i];
		tileClass->add(p.x, p.y);
	}
}