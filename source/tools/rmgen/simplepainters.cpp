#include "stdafx.h"
#include "simplepainters.h"

using namespace std;

TerrainPainter::TerrainPainter(const string& terrain)
{
    this->terrain = terrain;
}

void TerrainPainter::paint(Map* m, Area* a) 
{
    int id = m->getId(terrain);
    for (int i=0; i<a->points.size(); i++) {
        Point p = a->points[i];
        m->terrain[p.x][p.y] = id;
    }
}
