#include "stdafx.h"
#include "tileclass.h"

using namespace std;

TileClass::TileClass(int mapSize) {
	this->mapSize = mapSize;
	for(int i=0; i<mapSize; i++) {
		rc.push_back(new RangeCount(mapSize));
	}
}

TileClass::~TileClass() {
	for(int i=0; i<mapSize; i++) {
		delete rc[i];
	}
}

void TileClass::add(int x, int y) {
	rc[y]->add(x, 1);

	tiles.insert(Point(x, y));
}

void TileClass::remove(int x, int y) {
	rc[y]->add(x, -1);

	if(rc[y]->get(x) == 0) {
		tiles.erase(Point(x, y));
	}
}

bool TileClass::hasTilesInRadius(float cx, float cy, float r) {
	// special check for really small classes
	if(tiles.size() < 4*r) {
		for(set<Point>::iterator it = tiles.begin(); it != tiles.end(); it++) {
			Point p = *it;
			float dx = p.x - cx;
			float dy = p.y - cy;
			if(dx*dx + dy*dy <= r*r) {
				return true;
			}
		}

		return false;
	}

	for(float y = cy-r; y <= cy+r; y++) {
		int iy = (int) y;
		if(iy < 0 || iy >= mapSize) {
			continue;
		}

		float dy = y - cy;
		float dx = sqrt(r*r - dy*dy);
		float x1 = cx - dx;
		float x2 = cx + dx;
		int minX = max(0, (int) x1);
		int maxX = min(mapSize-1, (int) x2);
		if(rc[iy]->get(minX, maxX+1) > 0) {
			return true;
		}
	}

	return false;
}